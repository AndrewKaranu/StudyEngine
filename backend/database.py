from typing import List, Dict
from models import Exam, StudentResult, Deck, Flashcard, Quiz, QuizQuestion

# In-memory storage for simplicity
exams_db: Dict[str, Exam] = {}
results_db: List[StudentResult] = []
decks_db: Dict[str, Deck] = {}
quizzes_db: Dict[str, Quiz] = {}

# Initialize with sample deck
sample_deck = Deck(
    id="soen422-concepts",
    title="SOEN 422 Concepts",
    cards=[
        Flashcard(front="What is the I2C address of the PCF8575?", back="0x20 (usually)"),
        Flashcard(front="What is the voltage of the ESP32 logic levels?", back="3.3V"),
        Flashcard(front="Which pin is used for the built-in LED?", back="GPIO 2"),
        Flashcard(front="What does SPI stand for?", back="Serial Peripheral Interface"),
        Flashcard(front="What is the maximum WiFi range indoors?", back="~45 meters (150 feet)"),
        Flashcard(front="What is the purpose of a pull-up resistor?", back="To ensure a known state (HIGH) when the line is floating"),
        Flashcard(front="What is the difference between polling and interrupts?", back="Polling checks status periodically; Interrupts trigger on events"),
        Flashcard(front="What is LVGL?", back="Light and Versatile Graphics Library"),
        Flashcard(front="What is the default baud rate for ESP32 bootloader?", back="115200"),
        Flashcard(front="How many cores does the ESP32 have?", back="2 (Dual Core Xtensa LX6)")
    ]
)
decks_db[sample_deck.id] = sample_deck

# Initialize with sample quizzes
sample_quiz_1 = Quiz(
    id="embedded-systems-101",
    title="Embedded Systems 101",
    questions=[
        QuizQuestion(id=1, type="mcq", text="Which communication protocol is asynchronous?", options=["SPI", "I2C", "UART", "CAN"], correct_answer="2"),
        QuizQuestion(id=2, type="short_answer", text="What does GPIO stand for?", correct_answer="General Purpose Input Output"),
        QuizQuestion(id=3, type="mcq", text="What is the typical operating voltage of an Arduino Uno?", options=["3.3V", "5V", "12V", "1.8V"], correct_answer="1"),
        QuizQuestion(id=4, type="short_answer", text="Name the pin used for serial data transmission.", correct_answer="TX"),
        QuizQuestion(id=5, type="short_answer", text="What is the unit of frequency?", correct_answer="Hertz")
    ]
)
quizzes_db[sample_quiz_1.id] = sample_quiz_1

sample_quiz_2 = Quiz(
    id="cpp-basics",
    title="C++ Basics",
    questions=[
        QuizQuestion(id=1, type="short_answer", text="Which keyword is used to define a class?", correct_answer="class"),
        QuizQuestion(id=2, type="mcq", text="Which operator is used to access members of a pointer?", options=[".", "->", "::", "&"], correct_answer="1"),
        QuizQuestion(id=3, type="short_answer", text="What is the entry point function of a C++ program?", correct_answer="main"),
        QuizQuestion(id=4, type="mcq", text="Which data type stores true/false values?", options=["int", "char", "bool", "void"], correct_answer="2")
    ]
)
quizzes_db[sample_quiz_2.id] = sample_quiz_2

def add_exam(exam: Exam):
    exams_db[exam.id] = exam

def get_all_exams() -> List[Exam]:
    return list(exams_db.values())

def get_exam(exam_id: str) -> Exam:
    return exams_db.get(exam_id)

def add_result(result: StudentResult):
    results_db.append(result)

def get_results() -> List[StudentResult]:
    return results_db

def get_all_decks() -> List[Deck]:
    return list(decks_db.values())

def get_deck(deck_id: str) -> Deck:
    return decks_db.get(deck_id)

def get_all_quizzes() -> List[Quiz]:
    return list(quizzes_db.values())

def get_quiz(quiz_id: str) -> Quiz:
    return quizzes_db.get(quiz_id)


