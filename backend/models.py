from pydantic import BaseModel
from typing import List, Optional

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
