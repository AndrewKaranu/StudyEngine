from fastapi import FastAPI, HTTPException
from typing import List
from models import Exam, StudentResult, Deck, Quiz
import database

app = FastAPI(title="StudyEngine API")

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

if __name__ == "__main__":
    import uvicorn
    # Run on 0.0.0.0 to be accessible by ESP32
    uvicorn.run(app, host="0.0.0.0", port=8000)
