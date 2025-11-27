"""
Quick script to test the API endpoints locally.
Run this after starting the server with: python -m uvicorn main:app --host 0.0.0.0 --port 8000
"""

import requests
import json

BASE_URL = "http://localhost:8000"

def test_upload_exam():
    """Upload the sample exam"""
    with open("sample_exam.json", "r") as f:
        exam_data = json.load(f)
    
    response = requests.post(f"{BASE_URL}/exams/upload", json=exam_data)
    print(f"Upload Exam: {response.status_code}")
    print(response.json())
    return response.status_code == 200

def test_list_exams():
    """List all available exams"""
    response = requests.get(f"{BASE_URL}/exams")
    print(f"\nList Exams: {response.status_code}")
    print(json.dumps(response.json(), indent=2))
    return response.status_code == 200

def test_get_exam(exam_id):
    """Get a specific exam by ID"""
    response = requests.get(f"{BASE_URL}/exams/{exam_id}")
    print(f"\nGet Exam '{exam_id}': {response.status_code}")
    if response.status_code == 200:
        print(json.dumps(response.json(), indent=2))
    return response.status_code == 200

def test_submit_result():
    """Submit a sample result"""
    result = {
        "exam_id": "soen422-midterm-2025",
        "student_name": "Test Student",
        "student_id": "40123456",
        "score": 4,
        "total_questions": 5,
        "answers": [0, 2, 1, 2, 1]
    }
    response = requests.post(f"{BASE_URL}/results", json=result)
    print(f"\nSubmit Result: {response.status_code}")
    print(response.json())
    return response.status_code == 200

def test_get_results():
    """Get all submitted results"""
    response = requests.get(f"{BASE_URL}/results")
    print(f"\nGet Results: {response.status_code}")
    print(json.dumps(response.json(), indent=2))
    return response.status_code == 200

if __name__ == "__main__":
    print("=" * 50)
    print("StudyEngine API Test")
    print("=" * 50)
    
    test_upload_exam()
    test_list_exams()
    test_get_exam("soen422-midterm-2025")
    test_submit_result()
    test_get_results()
    
    print("\n" + "=" * 50)
    print("All tests completed!")
    print("=" * 50)
