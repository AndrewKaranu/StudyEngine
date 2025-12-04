from fastapi import FastAPI, HTTPException, UploadFile, File, Form, BackgroundTasks
from fastapi.middleware.cors import CORSMiddleware
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
    else:
        raise HTTPException(status_code=400, detail="Unknown generation type")

if __name__ == "__main__":
    import uvicorn
    # Run on 0.0.0.0 to be accessible by ESP32
    uvicorn.run(app, host="0.0.0.0", port=8000)
