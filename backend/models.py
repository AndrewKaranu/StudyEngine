from pydantic import BaseModel
from typing import List, Optional, Dict, Any
from enum import Enum
from datetime import datetime

class Question(BaseModel):
    id: int
    text: str
    options: List[str]  # ["A) ...", "B) ...", "C) ...", "D) ..."]
    correct_option: int # 0 for A, 1 for B, etc.

class Exam(BaseModel):
    id: str
    title: str
    duration_minutes: int
    show_results_immediate: bool
    questions: List[Question]

class StudentResult(BaseModel):
    exam_id: str
    student_name: str
    student_id: str
    score: int
    total_questions: int
    answers: List[int] # List of selected option indices

class Flashcard(BaseModel):
    front: str
    back: str

class Deck(BaseModel):
    id: str
    title: str
    cards: List[Flashcard]

class QuizQuestion(BaseModel):
    id: int
    type: str # "mcq" or "short_answer"
    text: str
    options: Optional[List[str]] = None
    correct_answer: str

class Quiz(BaseModel):
    id: str
    title: str
    questions: List[QuizQuestion]

# =============================================================================
# AI Generation Models
# =============================================================================

class ModelChoice(str, Enum):
    HAIKU = "claude-3-5-haiku-latest"
    SONNET = "claude-sonnet-4-20250514"

class GenerationType(str, Enum):
    QUIZ = "quiz"
    FLASHCARDS = "flashcards"

class JobStatus(str, Enum):
    PENDING = "pending"
    PROCESSING = "processing"
    COMPLETED = "completed"
    FAILED = "failed"

class GenerationRequest(BaseModel):
    """Request parameters for AI content generation"""
    generation_type: GenerationType
    model: ModelChoice = ModelChoice.SONNET
    title: Optional[str] = None  # Auto-generated if not provided
    
    # Quiz-specific options
    mcq_count: int = 5
    short_answer_count: int = 0
    
    # Flashcard-specific options
    flashcard_count: int = 10
    
    # Custom instructions for the AI
    custom_instructions: Optional[str] = None

class GenerationJob(BaseModel):
    """Tracks the status of an async generation job"""
    job_id: str
    status: JobStatus
    generation_type: GenerationType
    created_at: datetime
    completed_at: Optional[datetime] = None
    error: Optional[str] = None
    result: Optional[Dict[str, Any]] = None  # The generated Quiz or Deck as dict
    progress_message: Optional[str] = None

class GenerationResponse(BaseModel):
    """Response when checking generation job status"""
    job_id: str
    status: JobStatus
    result: Optional[Dict[str, Any]] = None
    error: Optional[str] = None
    created_at: datetime
    completed_at: Optional[datetime] = None
