import urllib.request
import zipfile
import os

model_url = "https://alphacephei.com/vosk/models/vosk-model-small-tr-0.3.zip"
zip_path = "vosk-model-small-tr.zip"
target_dir = "vosk-model"

if os.path.exists(target_dir):
    print(f"'{target_dir}' klasörü zaten var. Model indirme işlemi atlanıyor.")
else:
    print("Vosk Türkçe modeli indiriliyor (yaklaşık 35 MB)... Lütfen bekleyin.")
    urllib.request.urlretrieve(model_url, zip_path)
    
    print("Model arşivden çıkarılıyor...")
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(".")
        
    print("Klasör yeniden adlandırılıyor...")
    os.rename("vosk-model-small-tr-0.3", target_dir)
    
    print("Geçici zip dosyası siliniyor...")
    os.remove(zip_path)
    
    print("İşlem tamam! Model başarıyla yüklendi.")
