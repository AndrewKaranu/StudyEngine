# StudyEngine

A portable, offline-capable study device built on the ESP32 (TTGO LoRa V1) platform. StudyEngine allows students to take quizzes, review flashcards, and complete exams without needing a computer or phone, while teachers can generate AI-powered study materials and track student results.

##  Features

- **Quiz Mode** - Practice with multiple choice and short answer questions
- **Flashcard Mode** - Review concepts with spaced repetition
- **Exam Mode** - Timed assessments with student ID entry and result submission
- **Transcript Mode** - Generate quizzes/flashcards from voice lecture transcripts
- **AI Generation** - Create study materials from PDF documents using Claude AI
- **Web Admin Interface** - Upload content and view results via browser
- **Offline Capable** - Study content is cached locally on the device
- **Audio/Visual Feedback** - Speaker beeps and RGB LED indicators

---

##  Hardware

- **Board**: TTGO LoRa32 V1 (ESP32 with built-in OLED)
- **Display**: 128x64 OLED (SSD1306) + External TFT via LVGL
- **Input**: Potentiometer (scrolling) + 2 Buttons (A/B for selection)
- **Feedback**: Passive buzzer (speaker) + RGB LED
- **Connectivity**: WiFi for syncing with backend server

---

##  Project Structure

### ESP32 Firmware (Arduino/C++)

| File | Purpose |
|------|---------|
| `StudyEngine.ino` | Main sketch - initializes all managers, runs the main state machine loop, handles mode switching between menu/quiz/flashcards/exam/transcripts/settings |
| `config.h` | Configuration constants - WiFi credentials, API URL, pin definitions, timing constants |
| `DisplayManager.h/cpp` | Manages the small OLED display for status messages and simple text output |
| `UIManager.h/cpp` | Manages the main TFT display using LVGL library - renders menus, questions, flashcards, and all UI screens |
| `UITheme.h/cpp` | LVGL theme configuration - colors, fonts, and styling for consistent UI appearance |
| `InputManager.h/cpp` | Handles potentiometer reading (for scrolling/selection) and button debouncing (A=select, B=back) |
| `NetworkManager.h/cpp` | WiFi connection management, HTTP requests to backend API, content fetching and submission |
| `WebManager.h/cpp` | Runs a local web server on the ESP32 - serves the admin HTML interface for uploading content |
| `SettingsManager.h/cpp` | Persists user settings to EEPROM/Preferences - WiFi config, API URL, mute option, theme |
| `QuizEngine.h/cpp` | Quiz mode state machine - fetches quizzes, displays questions, tracks score, handles MCQ and short answer |
| `FlashcardEngine.h/cpp` | Flashcard mode - fetches decks, shows front/back cards, tracks progress |
| `ExamEngine.h/cpp` | Exam mode - student ID entry, timed questions, submits answers to backend for teacher review |
| `TranscriptEngine.h/cpp` | Transcript mode - displays voice transcripts, generates quizzes/flashcards via AI from transcript text |
| `FocusManager.h/cpp` | Pomodoro-style focus timer with work/break intervals |
| `StudyManager.h/cpp` | Tracks overall study statistics and progress |

### Backend Server (Python/FastAPI)

| File | Purpose |
|------|---------|
| `main.py` | FastAPI application - defines all REST API endpoints for content CRUD, AI generation, and result submission |
| `database.py` | SQLite database operations - stores quizzes, flashcard decks, exams, and student results |
| `ai_generator.py` | Claude AI integration - extracts text from PDFs, builds prompts, generates quizzes/flashcards/exams |
| `models.py` | Pydantic data models - defines request/response schemas for API validation |
| `requirements.txt` | Python dependencies - FastAPI, Anthropic SDK, PyMuPDF, etc. |
| `.env.example` | Example environment variables - copy to `.env` and add your Anthropic API key |
| `sample_exam.json` | Example exam JSON format for reference |
| `test_api.py` | API test script for development |
| `test_ui.html` | Standalone web UI for testing without the ESP32 device |

---

##  Getting Started

### Backend Setup

1. Navigate to the backend folder:
   ```bash
   cd backend
   ```

2. Create a virtual environment and install dependencies:
   ```bash
   python -m venv venv
   venv\Scripts\activate  # Windows
   pip install -r requirements.txt
   ```

3. Create `.env` file with your Anthropic API key:
   ```
   ANTHROPIC_API_KEY=sk-ant-...
   ```

4. Run the server:
   ```bash
   python main.py
   ```
   Server runs on `http://localhost:8000`

### ESP32 Setup

1. Install required Arduino libraries:
   - TFT_eSPI
   - LVGL
   - ArduinoJson
   - WiFi (built-in)

2. Configure `config.h`:
   ```cpp
   #define WIFI_SSID "YourNetwork"
   #define WIFI_PASSWORD "YourPassword"
   #define API_BASE_URL "http://YOUR_COMPUTER_IP:8000"
   ```

3. Upload to ESP32 via Arduino IDE or PlatformIO

### Testing Without Hardware

Open `backend/test_ui.html` in a browser to test the web interface without the ESP32 device.

---

##  Data Flow Examples

### Taking a Quiz
1. User selects "Quizzes" from main menu
2. `QuizEngine` calls `NetworkManager` to fetch quiz list from `/quizzes`
3. User selects a quiz → fetch full quiz from `/quiz/{id}`
4. Questions displayed via `UIManager` (LVGL)
5. User answers using potentiometer (scroll) and button A (select)
6. Score calculated and displayed at end

### AI Generation (Web)
1. Teacher opens device IP in browser → `WebManager` serves HTML
2. Teacher uploads PDF and clicks "Generate Quiz"
3. Browser POSTs to `/generate/quiz` with PDF file
4. `ai_generator.py` extracts text, sends to Claude API
5. Browser polls `/generate/status/{job_id}` until complete
6. Teacher clicks "Save" → POST to `/generate/save/{job_id}`
7. Quiz stored in SQLite, available on device

### Transcript Generation (Device)
1. User selects "Transcripts" from menu
2. `TranscriptEngine` displays available transcripts
3. User selects transcript and chooses "Generate Quiz"
4. Device POSTs transcript text to `/generate/transcript/quiz`
5. Polls for completion, then saves to database
6. New quiz available in Quiz Mode

---

## 🔊 Feedback System

- **Speaker** (can be muted in settings):
  - Click beep on button press
  - Success melody on correct answer/completion
  - Error buzz on wrong answer/failure

- **RGB LED**:
  - Green pulse during AI generation
  - Green flash on success
  - Red flash on error
  - Off when idle

---

This project was created for SOEN 422 - Embedded Systems at Concordia University.
