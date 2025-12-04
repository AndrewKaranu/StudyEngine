#include "WebManager.h"

// HTML Content stored in Flash Memory
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>StudyEngine Admin</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
  <style>
    body { background-color: #f8f9fa; }
    .container { max-width: 900px; margin-top: 30px; }
    .card { margin-bottom: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
    .progress-container { display: none; margin-top: 15px; }
    .status-badge { font-size: 0.9rem; }
    .generation-result { margin-top: 15px; padding: 15px; background: #f0f0f0; border-radius: 8px; display: none; }
    .result-preview { max-height: 300px; overflow-y: auto; background: white; padding: 10px; border-radius: 5px; font-family: monospace; font-size: 0.85rem; }
  </style>
</head>
<body>

  <div class="container">
    <nav class="navbar navbar-light bg-white rounded p-3 mb-4">
      <span class="navbar-brand mb-0 h1">StudyEngine Device Manager</span>
      <span class="badge bg-success">Connected to Device</span>
    </nav>

    <!-- Upload Section -->
    <div class="card">
      <div class="card-header bg-primary text-white">Upload Content</div>
      <div class="card-body">
        <ul class="nav nav-tabs" id="myTab" role="tablist">
          <li class="nav-item"><button class="nav-link active" data-bs-toggle="tab" data-bs-target="#quiz">Quiz</button></li>
          <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#exam">Exam</button></li>
          <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#deck">Flashcards</button></li>
        </ul>
        <div class="tab-content pt-3">
          <div class="tab-pane fade show active" id="quiz">
            <p class="text-muted">Upload a JSON quiz file to the backend.</p>
            <input type="file" id="quizFile" class="form-control mb-2" accept=".json">
            <button onclick="upload('quiz')" class="btn btn-success">Upload Quiz</button>
          </div>
          <div class="tab-pane fade" id="exam">
            <p class="text-muted">Upload a JSON exam file to the backend.</p>
            <input type="file" id="examFile" class="form-control mb-2" accept=".json">
            <button onclick="upload('exam')" class="btn btn-success">Upload Exam</button>
          </div>
          <div class="tab-pane fade" id="deck">
            <p class="text-muted">Upload a JSON flashcard deck file to the backend.</p>
            <input type="file" id="deckFile" class="form-control mb-2" accept=".json">
            <button onclick="upload('deck')" class="btn btn-success">Upload Deck</button>
          </div>
        </div>
      </div>
    </div>

    <!-- AI Generation Section -->
    <div class="card">
      <div class="card-header bg-purple text-white" style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);">
        <span>🤖 Generate from Notes (AI)</span>
      </div>
      <div class="card-body">
        <p class="text-muted">Upload your PDF notes and let AI generate quizzes or flashcards automatically.</p>
        
        <ul class="nav nav-tabs mb-3" role="tablist">
          <li class="nav-item"><button class="nav-link active" data-bs-toggle="tab" data-bs-target="#gen-quiz">Generate Quiz</button></li>
          <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#gen-flashcards">Generate Flashcards</button></li>
        </ul>
        
        <div class="tab-content">
          <!-- Generate Quiz Tab -->
          <div class="tab-pane fade show active" id="gen-quiz">
            <div class="row">
              <div class="col-md-6">
                <label class="form-label">PDF File</label>
                <input type="file" id="genQuizPdf" class="form-control mb-2" accept=".pdf">
              </div>
              <div class="col-md-6">
                <label class="form-label">Quiz Title</label>
                <input type="text" id="genQuizTitle" class="form-control mb-2" placeholder="e.g., Chapter 5 Review">
              </div>
            </div>
            <div class="row">
              <div class="col-md-4">
                <label class="form-label">AI Model</label>
                <select id="genQuizModel" class="form-select mb-2">
                  <option value="haiku">Haiku (Fast & Cheap)</option>
                  <option value="sonnet">Sonnet (Higher Quality)</option>
                </select>
              </div>
              <div class="col-md-4">
                <label class="form-label"># Multiple Choice</label>
                <input type="number" id="genQuizMcq" class="form-control mb-2" value="5" min="0" max="20">
              </div>
              <div class="col-md-4">
                <label class="form-label"># Short Answer</label>
                <input type="number" id="genQuizShort" class="form-control mb-2" value="3" min="0" max="10">
              </div>
            </div>
            <div class="mb-3">
              <label class="form-label">Custom Instructions (Optional)</label>
              <textarea id="genQuizInstructions" class="form-control" rows="2" placeholder="e.g., Focus on definitions, avoid calculations..."></textarea>
            </div>
            <button onclick="generateQuiz()" id="genQuizBtn" class="btn btn-primary">
              <span class="spinner-border spinner-border-sm d-none" id="genQuizSpinner"></span>
              Generate Quiz
            </button>
            
            <div class="progress-container" id="quizProgressContainer">
              <div class="d-flex align-items-center gap-2 mb-2">
                <span>Status:</span>
                <span id="quizStatus" class="badge bg-info status-badge">Processing...</span>
              </div>
              <div class="progress">
                <div class="progress-bar progress-bar-striped progress-bar-animated" id="quizProgressBar" style="width: 0%"></div>
              </div>
            </div>
            
            <div class="generation-result" id="quizResult">
              <h6>Generated Quiz:</h6>
              <div class="result-preview" id="quizResultPreview"></div>
              <button onclick="saveGenerated('quiz')" class="btn btn-success mt-2">Save to StudyEngine</button>
            </div>
          </div>
          
          <!-- Generate Flashcards Tab -->
          <div class="tab-pane fade" id="gen-flashcards">
            <div class="row">
              <div class="col-md-6">
                <label class="form-label">PDF File</label>
                <input type="file" id="genFlashPdf" class="form-control mb-2" accept=".pdf">
              </div>
              <div class="col-md-6">
                <label class="form-label">Deck Title</label>
                <input type="text" id="genFlashTitle" class="form-control mb-2" placeholder="e.g., Biology Chapter 3">
              </div>
            </div>
            <div class="row">
              <div class="col-md-6">
                <label class="form-label">AI Model</label>
                <select id="genFlashModel" class="form-select mb-2">
                  <option value="haiku">Haiku (Fast & Cheap)</option>
                  <option value="sonnet">Sonnet (Higher Quality)</option>
                </select>
              </div>
              <div class="col-md-6">
                <label class="form-label"># Flashcards</label>
                <input type="number" id="genFlashCount" class="form-control mb-2" value="10" min="1" max="50">
              </div>
            </div>
            <div class="mb-3">
              <label class="form-label">Custom Instructions (Optional)</label>
              <textarea id="genFlashInstructions" class="form-control" rows="2" placeholder="e.g., Include diagrams descriptions, focus on formulas..."></textarea>
            </div>
            <button onclick="generateFlashcards()" id="genFlashBtn" class="btn btn-primary">
              <span class="spinner-border spinner-border-sm d-none" id="genFlashSpinner"></span>
              Generate Flashcards
            </button>
            
            <div class="progress-container" id="flashProgressContainer">
              <div class="d-flex align-items-center gap-2 mb-2">
                <span>Status:</span>
                <span id="flashStatus" class="badge bg-info status-badge">Processing...</span>
              </div>
              <div class="progress">
                <div class="progress-bar progress-bar-striped progress-bar-animated" id="flashProgressBar" style="width: 0%"></div>
              </div>
            </div>
            
            <div class="generation-result" id="flashResult">
              <h6>Generated Flashcards:</h6>
              <div class="result-preview" id="flashResultPreview"></div>
              <button onclick="saveGenerated('flashcards')" class="btn btn-success mt-2">Save to StudyEngine</button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Results Section -->
    <div class="card">
      <div class="card-header bg-info text-white">Student Results</div>
      <div class="card-body">
        <button onclick="loadResults()" class="btn btn-sm btn-outline-primary mb-3">Refresh Results</button>
        <div class="table-responsive">
          <table class="table table-hover">
            <thead><tr><th>Student ID</th><th>Exam ID</th><th>Score</th></tr></thead>
            <tbody id="resultsBody"></tbody>
          </table>
        </div>
      </div>
    </div>
  </div>

  <script>
    // INJECTED BY ESP32
    const API_URL = "%API_URL%"; 
    let currentQuizJobId = null;
    let currentFlashJobId = null;
    let quizPollInterval = null;
    let flashPollInterval = null;

    async function upload(type) {
      const fileInput = document.getElementById(type + 'File');
      if(fileInput.files.length === 0) return alert("Select a file!");

      const formData = new FormData();
      formData.append("file", fileInput.files[0]);

      try {
        const res = await fetch(`${API_URL}/admin/upload/${type}`, {
          method: 'POST',
          body: formData
        });
        const data = await res.json();
        if(res.ok) alert("Success: " + data.message);
        else alert("Error: " + data.detail);
      } catch (e) {
        alert("Upload failed. Is the Python server running?");
        console.error(e);
      }
    }

    async function generateQuiz() {
      const pdfFile = document.getElementById('genQuizPdf').files[0];
      const title = document.getElementById('genQuizTitle').value;
      if(!pdfFile) return alert("Please select a PDF file!");
      if(!title) return alert("Please enter a quiz title!");

      const formData = new FormData();
      formData.append("file", pdfFile);
      formData.append("title", title);
      formData.append("model", document.getElementById('genQuizModel').value);
      formData.append("num_mcq", document.getElementById('genQuizMcq').value);
      formData.append("num_short_answer", document.getElementById('genQuizShort').value);
      const instructions = document.getElementById('genQuizInstructions').value;
      if(instructions) formData.append("custom_instructions", instructions);

      try {
        document.getElementById('genQuizBtn').disabled = true;
        document.getElementById('genQuizSpinner').classList.remove('d-none');
        document.getElementById('quizProgressContainer').style.display = 'block';
        document.getElementById('quizResult').style.display = 'none';
        document.getElementById('quizStatus').textContent = 'Starting...';
        document.getElementById('quizStatus').className = 'badge bg-info status-badge';
        document.getElementById('quizProgressBar').style.width = '10%';

        const res = await fetch(`${API_URL}/generate/quiz`, {
          method: 'POST',
          body: formData
        });
        const data = await res.json();
        
        if(!res.ok) {
          throw new Error(data.detail || 'Generation failed');
        }

        currentQuizJobId = data.job_id;
        document.getElementById('quizStatus').textContent = 'Processing PDF...';
        document.getElementById('quizProgressBar').style.width = '30%';
        
        // Start polling for status
        quizPollInterval = setInterval(() => pollQuizStatus(), 2000);
        
      } catch (e) {
        alert("Generation failed: " + e.message);
        resetQuizUI();
      }
    }

    async function pollQuizStatus() {
      if(!currentQuizJobId) return;
      
      try {
        const res = await fetch(`${API_URL}/generate/status/${currentQuizJobId}`);
        const data = await res.json();
        
        if(data.status === 'processing') {
          document.getElementById('quizStatus').textContent = 'AI is generating...';
          document.getElementById('quizProgressBar').style.width = '60%';
        } else if(data.status === 'completed') {
          clearInterval(quizPollInterval);
          document.getElementById('quizStatus').textContent = 'Completed!';
          document.getElementById('quizStatus').className = 'badge bg-success status-badge';
          document.getElementById('quizProgressBar').style.width = '100%';
          
          // Show result
          document.getElementById('quizResult').style.display = 'block';
          document.getElementById('quizResultPreview').textContent = JSON.stringify(data.result, null, 2);
          
          resetQuizUI();
        } else if(data.status === 'failed') {
          clearInterval(quizPollInterval);
          document.getElementById('quizStatus').textContent = 'Failed: ' + (data.error || 'Unknown error');
          document.getElementById('quizStatus').className = 'badge bg-danger status-badge';
          resetQuizUI();
        }
      } catch(e) {
        console.error('Poll error:', e);
      }
    }

    function resetQuizUI() {
      document.getElementById('genQuizBtn').disabled = false;
      document.getElementById('genQuizSpinner').classList.add('d-none');
    }

    async function generateFlashcards() {
      const pdfFile = document.getElementById('genFlashPdf').files[0];
      const title = document.getElementById('genFlashTitle').value;
      if(!pdfFile) return alert("Please select a PDF file!");
      if(!title) return alert("Please enter a deck title!");

      const formData = new FormData();
      formData.append("file", pdfFile);
      formData.append("title", title);
      formData.append("model", document.getElementById('genFlashModel').value);
      formData.append("num_flashcards", document.getElementById('genFlashCount').value);
      const instructions = document.getElementById('genFlashInstructions').value;
      if(instructions) formData.append("custom_instructions", instructions);

      try {
        document.getElementById('genFlashBtn').disabled = true;
        document.getElementById('genFlashSpinner').classList.remove('d-none');
        document.getElementById('flashProgressContainer').style.display = 'block';
        document.getElementById('flashResult').style.display = 'none';
        document.getElementById('flashStatus').textContent = 'Starting...';
        document.getElementById('flashStatus').className = 'badge bg-info status-badge';
        document.getElementById('flashProgressBar').style.width = '10%';

        const res = await fetch(`${API_URL}/generate/flashcards`, {
          method: 'POST',
          body: formData
        });
        const data = await res.json();
        
        if(!res.ok) {
          throw new Error(data.detail || 'Generation failed');
        }

        currentFlashJobId = data.job_id;
        document.getElementById('flashStatus').textContent = 'Processing PDF...';
        document.getElementById('flashProgressBar').style.width = '30%';
        
        // Start polling for status
        flashPollInterval = setInterval(() => pollFlashStatus(), 2000);
        
      } catch (e) {
        alert("Generation failed: " + e.message);
        resetFlashUI();
      }
    }

    async function pollFlashStatus() {
      if(!currentFlashJobId) return;
      
      try {
        const res = await fetch(`${API_URL}/generate/status/${currentFlashJobId}`);
        const data = await res.json();
        
        if(data.status === 'processing') {
          document.getElementById('flashStatus').textContent = 'AI is generating...';
          document.getElementById('flashProgressBar').style.width = '60%';
        } else if(data.status === 'completed') {
          clearInterval(flashPollInterval);
          document.getElementById('flashStatus').textContent = 'Completed!';
          document.getElementById('flashStatus').className = 'badge bg-success status-badge';
          document.getElementById('flashProgressBar').style.width = '100%';
          
          // Show result
          document.getElementById('flashResult').style.display = 'block';
          document.getElementById('flashResultPreview').textContent = JSON.stringify(data.result, null, 2);
          
          resetFlashUI();
        } else if(data.status === 'failed') {
          clearInterval(flashPollInterval);
          document.getElementById('flashStatus').textContent = 'Failed: ' + (data.error || 'Unknown error');
          document.getElementById('flashStatus').className = 'badge bg-danger status-badge';
          resetFlashUI();
        }
      } catch(e) {
        console.error('Poll error:', e);
      }
    }

    function resetFlashUI() {
      document.getElementById('genFlashBtn').disabled = false;
      document.getElementById('genFlashSpinner').classList.add('d-none');
    }

    async function saveGenerated(type) {
      const jobId = type === 'quiz' ? currentQuizJobId : currentFlashJobId;
      if(!jobId) return alert("No generation to save!");

      try {
        const res = await fetch(`${API_URL}/generate/save/${jobId}`, { method: 'POST' });
        const data = await res.json();
        if(res.ok) {
          alert("Success: " + data.message);
        } else {
          alert("Error: " + data.detail);
        }
      } catch(e) {
        alert("Save failed: " + e.message);
      }
    }

    async function loadResults() {
      try {
        const res = await fetch(`${API_URL}/results`);
        const data = await res.json();
        const tbody = document.getElementById('resultsBody');
        tbody.innerHTML = '';
        data.forEach(row => {
          tbody.innerHTML += `<tr><td>${row.student_id || 'Unknown'}</td><td>${row.exam_id}</td><td>${row.score}/${row.total_questions}</td></tr>`;
        });
      } catch (e) {
        console.error("Failed to load results", e);
      }
    }

    // Load results on page load
    loadResults();
  </script>
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/js/bootstrap.bundle.min.js"></script>
</body>
</html>
)rawliteral";

WebManager::WebManager(SENetworkManager* nm) : server(80), netMgr(nm) {}

void WebManager::begin() {
    server.on("/", [this]() { handleRoot(); });
    server.begin();
}

void WebManager::update() {
    server.handleClient();
}

void WebManager::handleRoot() {
    String html = FPSTR(INDEX_HTML);
    // Inject the Python Backend URL into the HTML
    String apiUrl = netMgr->getApiBaseUrl(); 
    html.replace("%API_URL%", apiUrl);
    
    server.send(200, "text/html", html);
}
