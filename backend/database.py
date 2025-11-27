from typing import List, Dict
from models import Exam, StudentResult, Deck, Flashcard

# In-memory storage for simplicity
exams_db: Dict[str, Exam] = {}
results_db: List[StudentResult] = []
decks_db: Dict[str, Deck] = {}

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

