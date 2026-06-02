import urllib.request
import zipfile
import os

model_url = "https://alphacephei.com/vosk/models/vosk-model-small-tr-0.3.zip"
zip_path = "vosk-model-small-tr.zip"
target_dir = "vosk-model"

if os.path.exists(target_dir):
    print(f"'{target_dir}' already exists. Skipping model download.")
else:
    print("Downloading Vosk Turkish model (approx. 35 MB)... Please wait.")
    urllib.request.urlretrieve(model_url, zip_path)
    
    print("Extracting model from zip archive...")
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(".")
        
    print("Renaming folder...")
    os.rename("vosk-model-small-tr-0.3", target_dir)
    
    print("Deleting temporary zip file...")
    os.remove(zip_path)
    
    print("Process completed! Model loaded successfully.")
