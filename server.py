
# from flask import Flask, request, jsonify

# app = Flask(__name__)

# # Lookup table with predefined distances for specific locations (in meters)
# lookup_table = [
#     {"location": "Start Point", "distance": 0},
#     {"location": "Turn 1", "distance": 5},
#     {"location": "Point A", "distance": 10},
#     {"location": "Turn 2", "distance": 15},
#     {"location": "Destination", "distance": 20}
# ]

# def find_nearest_location(traveled_distance):
#     closest_location = None
#     min_difference = float("inf")
    
#     for entry in lookup_table:
#         difference = abs(traveled_distance - entry["distance"])
#         if difference < min_difference:
#             min_difference = difference
#             closest_location = entry["location"]
    
#     return closest_location

# @app.route('/sensor', methods=['POST'])
# def receive_data():
#     data = request.json
#     traveled_distance = data.get("distance")
    
#     # Find the closest location based on the traveled distance
#     nearest_location = find_nearest_location(traveled_distance)

#     # Log the data for debugging
#     print(f"Traveled Distance: {traveled_distance} m, Nearest Location: {nearest_location}")
    
#     # Return the nearest location to the ESP32
#     return jsonify({"nearest_location": nearest_location})

# if __name__ == '__main__':
# app.run(host='0.0.0.0', port=5000)
from flask import Flask, request, jsonify
import firebase_admin
from firebase_admin import credentials, firestore
import os

# Initialize Firebase Admin SDK with credentials
cred = credentials.Certificate("dosc-3724d-firebase-adminsdk-2ml6h-99f2b3e2dc.json")  # Use your actual file path here
firebase_admin.initialize_app(cred)

# Initialize Firestore
db = firestore.client()

# Flask app initialization
app = Flask(__name__)

# Lookup table with predefined distances for specific locations (in meters)
lookup_table = [
    {"location": "Start Point", "distance": 0},
    {"location": "Turn 1", "distance": -250},
    {"location": "Point A", "distance": -500},
    {"location": "Turn 2", "distance": 800},
    {"location": "STOP 1", "distance": 1020}
]

def find_nearest_location(traveled_distance):
    closest_location = None
    min_difference = float("inf")
    
    for entry in lookup_table:
        difference = abs(traveled_distance - entry["distance"])
        if difference < min_difference:
            min_difference = difference
            closest_location = entry["location"]
    
    return closest_location

@app.route('/sensor', methods=['POST'])
def receive_data():
    data = request.json
    traveled_distance = data.get("distance")
    
    # Find the closest location based on the traveled distance
    nearest_location = find_nearest_location(traveled_distance)

    # Log the data for debugging
    print(f"Traveled Distance: {traveled_distance} m, Nearest Location: {nearest_location}")
    
    # Write data to Firebase
    data_to_save = {
        "distance": traveled_distance,
        "nearest_location": nearest_location
    }
    # Save the data to Firestore
    db.collection("sensor_data").add(data_to_save)

    # Return the nearest location to the ESP32
    return jsonify({"nearest_location": nearest_location})

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
