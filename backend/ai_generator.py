"""
AI Generator Service - Uses Claude API to generate quizzes and flashcards from PDFs
"""

import anthropic
import base64
import json
import os
import uuid
import asyncio
from datetime import datetime
from typing import Optional, Dict, Any
from models import (
    GenerationRequest, GenerationJob, JobStatus, GenerationType,
    ModelChoice, Quiz, QuizQuestion, Deck, Flashcard
)

# In-memory job storage (could be replaced with Redis/DB for production)
jobs: Dict[str, GenerationJob] = {}


def get_client() -> anthropic.Anthropic:
    """Get Anthropic client with API key from environment"""
    api_key = os.getenv("ANTHROPIC_API_KEY")
    if not api_key:
        raise ValueError("ANTHROPIC_API_KEY environment variable not set")
    return anthropic.Anthropic(api_key=api_key)


def create_job(generation_type: GenerationType) -> GenerationJob:
    """Create a new generation job"""
    job = GenerationJob(
        job_id=str(uuid.uuid4()),
        status=JobStatus.PENDING,
        generation_type=generation_type,
        created_at=datetime.now(),
        progress_message="Job created, waiting to start..."
    )
    jobs[job.job_id] = job
    return job


def get_job(job_id: str) -> Optional[GenerationJob]:
    """Get a job by ID"""
    return jobs.get(job_id)


def update_job(job_id: str, **kwargs):
    """Update job fields"""
    if job_id in jobs:
        job_dict = jobs[job_id].model_dump()
        job_dict.update(kwargs)
        jobs[job_id] = GenerationJob(**job_dict)


def build_quiz_prompt(request: GenerationRequest) -> str:
    """Build the prompt for quiz generation"""
    total_questions = request.mcq_count + request.short_answer_count
    
    prompt = f"""Analyze the provided PDF document and generate a quiz based on its content.

REQUIREMENTS:
- Generate exactly {request.mcq_count} multiple choice questions (MCQ)
- Generate exactly {request.short_answer_count} short answer questions
- Total: {total_questions} questions
- Questions should test understanding of key concepts from the document
- MCQ questions must have exactly 4 options (A, B, C, D)
- Vary difficulty from easy to challenging
- Ensure questions are clear and unambiguous

{f"ADDITIONAL INSTRUCTIONS: {request.custom_instructions}" if request.custom_instructions else ""}

OUTPUT FORMAT:
You must respond with ONLY valid JSON matching this exact structure (no markdown, no explanation):

{{
  "title": "Quiz title based on document content",
  "questions": [
    {{
      "id": 1,
      "type": "mcq",
      "text": "Question text here?",
      "options": ["A) First option", "B) Second option", "C) Third option", "D) Fourth option"],
      "correct_answer": "A"
    }},
    {{
      "id": 2,
      "type": "short_answer",
      "text": "Short answer question here?",
      "options": null,
      "correct_answer": "Expected answer text"
    }}
  ]
}}

Generate the quiz now based on the PDF content:"""
    
    return prompt


def build_flashcard_prompt(request: GenerationRequest) -> str:
    """Build the prompt for flashcard generation"""
    
    prompt = f"""Analyze the provided PDF document and generate flashcards for studying its content.

REQUIREMENTS:
- Generate exactly {request.flashcard_count} flashcards
- Focus on key concepts, definitions, and important facts
- Front of card should be a question or term
- Back of card should be a clear, concise answer or definition
- Cover the most important topics from the document
- Vary the types: definitions, concepts, processes, relationships

{f"ADDITIONAL INSTRUCTIONS: {request.custom_instructions}" if request.custom_instructions else ""}

OUTPUT FORMAT:
You must respond with ONLY valid JSON matching this exact structure (no markdown, no explanation):

{{
  "title": "Deck title based on document content",
  "cards": [
    {{
      "front": "What is [concept]?",
      "back": "Clear definition or explanation here"
    }},
    {{
      "front": "Term or question",
      "back": "Answer or definition"
    }}
  ]
}}

Generate the flashcards now based on the PDF content:"""
    
    return prompt


def parse_quiz_response(response_text: str, request: GenerationRequest) -> Quiz:
    """Parse Claude's response into a Quiz object"""
    # Clean up response (remove any markdown code blocks if present)
    clean_text = response_text.strip()
    if clean_text.startswith("```"):
        # Remove markdown code block
        lines = clean_text.split("\n")
        clean_text = "\n".join(lines[1:-1] if lines[-1] == "```" else lines[1:])
    
    data = json.loads(clean_text)
    
    # Generate unique quiz ID
    quiz_id = f"quiz_{uuid.uuid4().hex[:8]}"
    
    # Convert to QuizQuestion objects
    questions = []
    for i, q in enumerate(data["questions"]):
        questions.append(QuizQuestion(
            id=i + 1,
            type=q["type"],
            text=q["text"],
            options=q.get("options"),
            correct_answer=q["correct_answer"]
        ))
    
    return Quiz(
        id=quiz_id,
        title=request.title or data.get("title", "Generated Quiz"),
        questions=questions
    )


def parse_flashcard_response(response_text: str, request: GenerationRequest) -> Deck:
    """Parse Claude's response into a Deck object"""
    # Clean up response
    clean_text = response_text.strip()
    if clean_text.startswith("```"):
        lines = clean_text.split("\n")
        clean_text = "\n".join(lines[1:-1] if lines[-1] == "```" else lines[1:])
    
    data = json.loads(clean_text)
    
    # Generate unique deck ID
    deck_id = f"deck_{uuid.uuid4().hex[:8]}"
    
    # Convert to Flashcard objects
    cards = [Flashcard(front=c["front"], back=c["back"]) for c in data["cards"]]
    
    return Deck(
        id=deck_id,
        title=request.title or data.get("title", "Generated Flashcards"),
        cards=cards
    )


async def generate_content(
    job_id: str,
    pdf_data: bytes,
    request: GenerationRequest
):
    """
    Async function to generate content from PDF using Claude API
    
    Args:
        job_id: The job ID to update
        pdf_data: Raw PDF bytes
        request: Generation request parameters
    """
    try:
        update_job(job_id, status=JobStatus.PROCESSING, progress_message="Encoding PDF...")
        
        # Encode PDF to base64
        pdf_base64 = base64.standard_b64encode(pdf_data).decode("utf-8")
        
        update_job(job_id, progress_message=f"Sending to Claude ({request.model.value})...")
        
        # Build prompt based on generation type
        if request.generation_type == GenerationType.QUIZ:
            prompt = build_quiz_prompt(request)
        else:
            prompt = build_flashcard_prompt(request)
        
        # Call Claude API
        client = get_client()
        
        message = client.messages.create(
            model=request.model.value,
            max_tokens=4096,
            messages=[
                {
                    "role": "user",
                    "content": [
                        {
                            "type": "document",
                            "source": {
                                "type": "base64",
                                "media_type": "application/pdf",
                                "data": pdf_base64
                            }
                        },
                        {
                            "type": "text",
                            "text": prompt
                        }
                    ]
                }
            ]
        )
        
        update_job(job_id, progress_message="Parsing response...")
        
        # Extract text response
        response_text = message.content[0].text
        
        # Parse response based on type
        if request.generation_type == GenerationType.QUIZ:
            result = parse_quiz_response(response_text, request)
        else:
            result = parse_flashcard_response(response_text, request)
        
        # Update job as completed with result stored
        update_job(
            job_id,
            status=JobStatus.COMPLETED,
            completed_at=datetime.now(),
            result=result.model_dump(),
            progress_message=f"Successfully generated {request.generation_type.value}!"
        )
        
        print(f"[AI] Job {job_id} completed: {result.id}")
        
    except json.JSONDecodeError as e:
        update_job(
            job_id,
            status=JobStatus.FAILED,
            completed_at=datetime.now(),
            error=f"Failed to parse AI response as JSON: {str(e)}",
            progress_message="Generation failed"
        )
        print(f"[AI] Job {job_id} failed: JSON parse error")
        
    except anthropic.APIError as e:
        update_job(
            job_id,
            status=JobStatus.FAILED,
            completed_at=datetime.now(),
            error=f"Claude API error: {str(e)}",
            progress_message="Generation failed"
        )
        print(f"[AI] Job {job_id} failed: API error - {e}")
        
    except Exception as e:
        update_job(
            job_id,
            status=JobStatus.FAILED,
            completed_at=datetime.now(),
            error=f"Unexpected error: {str(e)}",
            progress_message="Generation failed"
        )
        print(f"[AI] Job {job_id} failed: {e}")


def start_generation_job(
    pdf_data: bytes,
    request: GenerationRequest
) -> str:
    """
    Start an async generation job
    
    Returns the job_id immediately while processing continues in background
    """
    job = create_job(request.generation_type)
    
    # Schedule the async task
    asyncio.create_task(generate_content(job.job_id, pdf_data, request))
    
    return job.job_id
