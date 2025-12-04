from fastapi import FastAPI, HTTPException, UploadFile, File, Form, BackgroundTasks
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from typing import List, Optional
from models import (
    Exam, StudentResult, Deck, Quiz, 
    GenerationRequest, GenerationJob, GenerationResponse,
    GenerationType, ModelChoice, JobStatus
)
import database
import json
import ai_generator
from dotenv import load_dotenv

# Load environment variables from .env file
load_dotenv()

app = FastAPI(title="StudyEngine API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

@app.get("/")
def read_root():
    return {"message": "Welcome to StudyEngine API"}

@app.post("/exams/upload")
def upload_exam(exam: Exam):
    database.add_exam(exam)
    return {"message": "Exam uploaded successfully", "exam_id": exam.id}

@app.get("/exams", response_model=List[Exam])
def list_exams():
    return database.get_all_exams()

@app.get("/exams/{exam_id}", response_model=Exam)
def get_exam(exam_id: str):
    exam = database.get_exam(exam_id)
    if not exam:
        raise HTTPException(status_code=404, detail="Exam not found")
    return exam

@app.post("/results")
def submit_result(result: StudentResult):
    database.add_result(result)
    print(f"Received result for {result.student_name}: {result.score}/{result.total_questions}")
    return {"message": "Result received"}

@app.get("/results", response_model=List[StudentResult])
def get_results():
    return database.get_results()

@app.get("/decks", response_model=List[Deck])
def list_decks():
    return database.get_all_decks()

@app.get("/decks/{deck_id}", response_model=Deck)
def get_deck(deck_id: str):
    deck = database.get_deck(deck_id)
    if not deck:
        raise HTTPException(status_code=404, detail="Deck not found")
    return deck

@app.get("/quizzes", response_model=List[Quiz])
def list_quizzes():
    return database.get_all_quizzes()

@app.get("/quizzes/{quiz_id}", response_model=Quiz)
def get_quiz(quiz_id: str):
    quiz = database.get_quiz(quiz_id)
    if not quiz:
        raise HTTPException(status_code=404, detail="Quiz not found")
    return quiz

@app.post("/admin/upload/exam")
async def admin_upload_exam(file: UploadFile = File(...)):
    try:
        content = await file.read()
        data = json.loads(content)
        exam = Exam(**data)
        database.add_exam(exam)
        return {"message": "Exam uploaded successfully", "id": exam.id}
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Invalid Exam JSON: {str(e)}")

@app.post("/admin/upload/quiz")
async def admin_upload_quiz(file: UploadFile = File(...)):
    try:
        content = await file.read()
        data = json.loads(content)
        quiz = Quiz(**data)
        database.add_quiz(quiz)
        return {"message": "Quiz uploaded successfully", "id": quiz.id}
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Invalid Quiz JSON: {str(e)}")

@app.post("/admin/upload/deck")
async def admin_upload_deck(file: UploadFile = File(...)):
    try:
        content = await file.read()
        data = json.loads(content)
        deck = Deck(**data)
        database.add_deck(deck)
        return {"message": "Deck uploaded successfully", "id": deck.id}
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Invalid Deck JSON: {str(e)}")

# ============== AI Content Generation Endpoints ==============

@app.post("/generate/quiz", response_model=dict)
async def generate_quiz_from_pdf(
    background_tasks: BackgroundTasks,
    file: UploadFile = File(...),
    title: str = Form(...),
    model: str = Form("haiku"),
    num_mcq: int = Form(5),
    num_short_answer: int = Form(3),
    custom_instructions: Optional[str] = Form(None)
):
    """
    Generate a quiz from a PDF file using Claude AI.
    Returns a job_id for polling the generation status.
    """
    # Validate file type
    if not file.filename.lower().endswith('.pdf'):
        raise HTTPException(status_code=400, detail="Only PDF files are supported")
    
    # Read PDF content
    try:
        pdf_content = await file.read()
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Error reading PDF: {str(e)}")
    
    # Validate file size (max 32MB for Claude)
    if len(pdf_content) > 32 * 1024 * 1024:
        raise HTTPException(status_code=400, detail="PDF file too large. Maximum size is 32MB.")
    
    # Parse model choice
    model_choice = ModelChoice.HAIKU if model.lower() == "haiku" else ModelChoice.SONNET
    
    # Create generation request
    request = GenerationRequest(
        title=title,
        generation_type=GenerationType.QUIZ,
        model=model_choice,
        mcq_count=num_mcq,
        short_answer_count=num_short_answer,
        custom_instructions=custom_instructions
    )
    
    # Start async generation job
    job_id = ai_generator.start_generation_job(pdf_content, request)
    
    return {"job_id": job_id, "message": "Quiz generation started"}

@app.post("/generate/flashcards", response_model=dict)
async def generate_flashcards_from_pdf(
    background_tasks: BackgroundTasks,
    file: UploadFile = File(...),
    title: str = Form(...),
    model: str = Form("haiku"),
    num_flashcards: int = Form(10),
    custom_instructions: Optional[str] = Form(None)
):
    """
    Generate flashcards from a PDF file using Claude AI.
    Returns a job_id for polling the generation status.
    """
    # Validate file type
    if not file.filename.lower().endswith('.pdf'):
        raise HTTPException(status_code=400, detail="Only PDF files are supported")
    
    # Read PDF content
    try:
        pdf_content = await file.read()
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Error reading PDF: {str(e)}")
    
    # Validate file size (max 32MB for Claude)
    if len(pdf_content) > 32 * 1024 * 1024:
        raise HTTPException(status_code=400, detail="PDF file too large. Maximum size is 32MB.")
    
    # Parse model choice
    model_choice = ModelChoice.HAIKU if model.lower() == "haiku" else ModelChoice.SONNET
    
    # Create generation request
    request = GenerationRequest(
        title=title,
        generation_type=GenerationType.FLASHCARDS,
        model=model_choice,
        flashcard_count=num_flashcards,
        custom_instructions=custom_instructions
    )
    
    # Start async generation job
    job_id = ai_generator.start_generation_job(pdf_content, request)
    
    return {"job_id": job_id, "message": "Flashcard generation started"}

@app.get("/generate/status/{job_id}", response_model=GenerationResponse)
async def get_generation_status(job_id: str):
    """
    Get the status of a generation job.
    Poll this endpoint to check if generation is complete.
    """
    job = ai_generator.get_job(job_id)
    if not job:
        raise HTTPException(status_code=404, detail="Job not found")
    
    return GenerationResponse(
        job_id=job.job_id,
        status=job.status,
        result=job.result,
        error=job.error,
        created_at=job.created_at,
        completed_at=job.completed_at
    )

@app.post("/generate/save/{job_id}")
async def save_generated_content(job_id: str):
    """
    Save the generated content (quiz or deck) to the database.
    Only works for completed jobs.
    """
    job = ai_generator.get_job(job_id)
    if not job:
        raise HTTPException(status_code=404, detail="Job not found")
    
    if job.status != JobStatus.COMPLETED:
        raise HTTPException(status_code=400, detail=f"Job is not completed. Status: {job.status.value}")
    
    if not job.result:
        raise HTTPException(status_code=400, detail="No result to save")
    
    # Save based on generation type
    if job.generation_type == GenerationType.QUIZ:
        quiz = Quiz(**job.result)
        database.add_quiz(quiz)
        return {"message": "Quiz saved successfully", "id": quiz.id}
    elif job.generation_type == GenerationType.FLASHCARDS:
        deck = Deck(**job.result)
        database.add_deck(deck)
        return {"message": "Deck saved successfully", "id": deck.id}
    elif job.generation_type == GenerationType.EXAM:
        exam = Exam(**job.result)
        database.add_exam(exam)
        return {"message": "Exam saved successfully", "id": exam.id}
    else:
        raise HTTPException(status_code=400, detail="Unknown generation type")


# ============== Transcript-based Generation Endpoints ==============

class TranscriptGenerationRequest(BaseModel):
    """Request body for transcript-based generation"""
    transcript_id: str
    transcript_content: str
    title: str
    model: str = "haiku"
    # Quiz options
    num_mcq: int = 5
    num_short_answer: int = 3
    # Flashcard options
    num_flashcards: int = 10
    custom_instructions: Optional[str] = None

@app.post("/generate/transcript/quiz", response_model=dict)
async def generate_quiz_from_transcript(request: TranscriptGenerationRequest):
    """
    Generate a quiz from transcript text using Claude AI.
    This is used by the ESP32 Transcript Mode.
    Returns a job_id for polling the generation status.
    """
    if not request.transcript_content or len(request.transcript_content.strip()) < 50:
        raise HTTPException(status_code=400, detail="Transcript content is too short")
    
    # Parse model choice
    model_choice = ModelChoice.HAIKU if request.model.lower() == "haiku" else ModelChoice.SONNET
    
    # Create generation request
    gen_request = GenerationRequest(
        title=request.title,
        generation_type=GenerationType.QUIZ,
        model=model_choice,
        mcq_count=request.num_mcq,
        short_answer_count=request.num_short_answer,
        custom_instructions=request.custom_instructions,
        transcript_content=request.transcript_content,
        transcript_id=request.transcript_id
    )
    
    # Start async generation job
    job_id = ai_generator.start_transcript_generation_job(gen_request)
    
    return {"job_id": job_id, "message": "Quiz generation from transcript started"}

@app.post("/generate/transcript/flashcards", response_model=dict)
async def generate_flashcards_from_transcript(request: TranscriptGenerationRequest):
    """
    Generate flashcards from transcript text using Claude AI.
    This is used by the ESP32 Transcript Mode.
    Returns a job_id for polling the generation status.
    """
    if not request.transcript_content or len(request.transcript_content.strip()) < 50:
        raise HTTPException(status_code=400, detail="Transcript content is too short")
    
    # Parse model choice
    model_choice = ModelChoice.HAIKU if request.model.lower() == "haiku" else ModelChoice.SONNET
    
    # Create generation request
    gen_request = GenerationRequest(
        title=request.title,
        generation_type=GenerationType.FLASHCARDS,
        model=model_choice,
        flashcard_count=request.num_flashcards,
        custom_instructions=request.custom_instructions,
        transcript_content=request.transcript_content,
        transcript_id=request.transcript_id
    )
    
    # Start async generation job
    job_id = ai_generator.start_transcript_generation_job(gen_request)
    
    return {"job_id": job_id, "message": "Flashcard generation from transcript started"}


# ============== Exam Generation from PDF ==============

@app.post("/generate/exam", response_model=dict)
async def generate_exam_from_pdf(
    background_tasks: BackgroundTasks,
    file: UploadFile = File(...),
    title: str = Form(...),
    model: str = Form("sonnet"),
    num_questions: int = Form(10),
    duration_minutes: int = Form(60),
    custom_instructions: Optional[str] = Form(None)
):
    """
    Generate an exam from a PDF file using Claude AI.
    These exams are for manual grading - answers are collected but not auto-scored.
    Returns a job_id for polling the generation status.
    """
    # Validate file type
    if not file.filename.lower().endswith('.pdf'):
        raise HTTPException(status_code=400, detail="Only PDF files are supported")
    
    # Read PDF content
    try:
        pdf_content = await file.read()
    except Exception as e:
        raise HTTPException(status_code=400, detail=f"Error reading PDF: {str(e)}")
    
    # Validate file size (max 32MB for Claude)
    if len(pdf_content) > 32 * 1024 * 1024:
        raise HTTPException(status_code=400, detail="PDF file too large. Maximum size is 32MB.")
    
    # Parse model choice
    model_choice = ModelChoice.HAIKU if model.lower() == "haiku" else ModelChoice.SONNET
    
    # Create generation request
    request = GenerationRequest(
        title=title,
        generation_type=GenerationType.EXAM,
        model=model_choice,
        exam_question_count=num_questions,
        exam_duration_minutes=duration_minutes,
        custom_instructions=custom_instructions
    )
    
    # Start async generation job
    job_id = ai_generator.start_generation_job(pdf_content, request)
    
    return {"job_id": job_id, "message": "Exam generation started"}


# ============== Student Answer Download ==============

@app.get("/results/{exam_id}/download")
async def download_exam_answers(exam_id: str):
    """
    Download all student answers for a specific exam.
    Returns a JSON with exam info and all student submissions.
    """
    # Get the exam
    exam = database.get_exam(exam_id)
    if not exam:
        raise HTTPException(status_code=404, detail="Exam not found")
    
    # Get all results for this exam
    all_results = database.get_results()
    exam_results = [r for r in all_results if r.exam_id == exam_id]
    
    if not exam_results:
        raise HTTPException(status_code=404, detail="No submissions found for this exam")
    
    # Build detailed answer report
    submissions = []
    for result in exam_results:
        student_answers = []
        for i, answer_idx in enumerate(result.answers):
            if i < len(exam.questions):
                q = exam.questions[i]
                answer_text = q.options[answer_idx] if 0 <= answer_idx < len(q.options) else "No answer"
                student_answers.append({
                    "question_num": i + 1,
                    "question_text": q.text,
                    "selected_option": answer_idx,
                    "selected_answer": answer_text,
                    "all_options": q.options
                })
        
        submissions.append({
            "student_id": result.student_id,
            "student_name": result.student_name,
            "score": result.score,
            "total_questions": result.total_questions,
            "answers": student_answers
        })
    
    return {
        "exam_id": exam.id,
        "exam_title": exam.title,
        "duration_minutes": exam.duration_minutes,
        "manual_grading": getattr(exam, 'manual_grading', False),
        "total_submissions": len(submissions),
        "submissions": submissions
    }

@app.get("/results/export/{exam_id}")
async def export_exam_answers_csv(exam_id: str):
    """
    Export student answers for an exam as CSV format.
    """
    exam = database.get_exam(exam_id)
    if not exam:
        raise HTTPException(status_code=404, detail="Exam not found")
    
    all_results = database.get_results()
    exam_results = [r for r in all_results if r.exam_id == exam_id]
    
    if not exam_results:
        raise HTTPException(status_code=404, detail="No submissions found for this exam")
    
    # Build CSV content
    csv_lines = []
    
    # Header row
    header = ["Student ID", "Student Name", "Score", "Total"]
    for i, q in enumerate(exam.questions):
        header.append(f"Q{i+1}")
    csv_lines.append(",".join(header))
    
    # Data rows
    for result in exam_results:
        row = [
            result.student_id,
            result.student_name,
            str(result.score),
            str(result.total_questions)
        ]
        for i, answer_idx in enumerate(result.answers):
            if i < len(exam.questions):
                q = exam.questions[i]
                answer_text = q.options[answer_idx] if 0 <= answer_idx < len(q.options) else "N/A"
                # Clean answer text for CSV
                answer_text = answer_text.replace(",", ";").replace('"', "'")
                row.append(f'"{answer_text}"')
            else:
                row.append("N/A")
        csv_lines.append(",".join(row))
    
    csv_content = "\n".join(csv_lines)
    
    return JSONResponse(
        content={"csv": csv_content, "filename": f"{exam_id}_answers.csv"},
        headers={"Content-Disposition": f"attachment; filename={exam_id}_answers.csv"}
    )

if __name__ == "__main__":
    import uvicorn
    # Run on 0.0.0.0 to be accessible by ESP32
    uvicorn.run(app, host="0.0.0.0", port=8000)
