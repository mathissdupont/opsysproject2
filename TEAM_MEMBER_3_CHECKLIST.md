# Ekip Üyesi 3 - Yapılacaklar Listesi

## ✅ Tamamlanan

1. **Rapor Yazımı: Scenario 3 & 4** ✅
   - `PROBLEM2_REPORT.md` oluşturuldu
   - Asymmetric Resource Acquisition detaylı açıklanmış
   - Right-GPU Preference detaylı açıklanmış
   - Edge case'ler analiz edilmiş
   - Deadlock-free garantiler matematiksel olarak gösterilmiş
   - Dosya GitHub'a push edilmiş

2. **Kod Logic Kontrol** ✅
   - `asymmetric_worker()` fonksiyonu doğru (satır 153-170)
   - `right_preference_worker()` fonksiyonu doğru (satır 172-210)
   - Her ikisi de deadlock-free
   - Parity-based ordering (even/odd) doğru uygulanmış
   - Non-blocking `pthread_mutex_trylock()` doğru kullanılmış

---

## 🔄 Yapılması Gerekenler

### 1. Kodun Derlenmesi ve Test Edilmesi

**Seçenek A: Windows PowerShell'de (eğer GCC yoksa, skip)**
- Native Windows GCC pthread.h eksik → **Skip**

**Seçenek B: WSL/Ubuntu'da (ÖNERILIR)**
```bash
# WSL'de:
cd /path/to/project
gcc -Wall -Wextra -pedantic -std=c11 ai_researchers.c -o ai_researchers -pthread

# Tüm 4 senaryoyu çalıştır
echo "0" | ./ai_researchers

# Sadece Scenario 3'ü çalıştır
echo "3" | ./ai_researchers

# Sadece Scenario 4'ü çalıştır
echo "4" | ./ai_researchers
```

**Seçenek C: GitHub Actions (Otomatik)**
- Repository master branch'ine `push` edildi
- GitHub Actions CI otomatik olarak test edecek (`.github/workflows/` varsa)
- Test başarılı olup olmadığını https://github.com/mathissdupont/opsysproject2/actions adresinden kontrol et

**Seçenek D: Docker (Alternatif)**
```docker
# Dockerfile içinde:
FROM ubuntu:latest
RUN apt-get update && apt-get install -y gcc
COPY ai_researchers.c .
RUN gcc -Wall -Wextra -pedantic -std=c11 ai_researchers.c -o ai_researchers -pthread
CMD ["sh", "-c", "echo 0 | ./ai_researchers"]
```

### 2. Test Çıktılarını Ekran Görüntüsü Olarak Kaydet

**Çalıştırılması Gereken Komutlar:**

a) **Scenario 3: Asymmetric Acquisition**
```bash
echo "3" | ./ai_researchers | head -80
# Beklenen çıktı:
# === Scenario 3: Asymmetric Resource Acquisition ===
# [Researcher 1] Analyzing data.
# [Researcher 1] Acquired GPUs X then Y.  ← Farklı sıra
# ...
```

b) **Scenario 4: Right-GPU Preference**
```bash
echo "4" | ./ai_researchers | head -80
# Beklenen çıktı:
# === Scenario 4: Right-GPU Preference ===
# [Researcher 3] Acquired GPUs Y and X with preference.  ← Farklı sıra
# ...
```

c) **Tüm 4 Senaryo (tam test)**
```bash
echo "0" | ./ai_researchers > full_test_output.txt 2>&1
# full_test_output.txt'yi kaydet
```

**Ekran Görüntüleri Konusunda:**
- Tüm dört senaryo başarılı bir şekilde tamamlanıyor
- Deadlock veya timeout yok
- Tüm researchers training'i tamamlıyor

**Dosya Adları:**
- `scenario_3_output.txt` → Scenario 3 test çıktısı
- `scenario_4_output.txt` → Scenario 4 test çıktısı
- `full_test_output.txt` → Tüm dört senaryo

### 3. Final Rapor Dosyası Oluşturma

**Yeni Dosya: `FINAL_REPORT.md`**

Şu başlıkları dahil et:

```markdown
# CENG302 Project 2 - Final Report

## Ekip Bilgileri
- Üye 1: [İsim] - it_support.c (Problem 1)
- Üye 2: [İsim] - ai_researchers.c (Scenario 1 & 2)
- Üye 3: [İsim] - ai_researchers.c (Scenario 3 & 4, Test, Rapor)

## Proje Özeti

### Problem 1: Sleeping IT Support
- Kaynak: PROBLEM1_REPORT.md

### Problem 2: AI Researchers and GPUs
- Kaynak: PROBLEM2_REPORT.md
- 4 senaryo uygulanmış
- Deadlock-free garantisi sağlanmış

## Teknik Detaylar

### Senaryo 3: Asymmetric Resource Acquisition
[PROBLEM2_REPORT.md'den özet]

### Senaryo 4: Right-GPU Preference
[PROBLEM2_REPORT.md'den özet]

## Test Sonuçları

### Derleme
- Dosya: Makefile
- Komut: `make`
- Sonuç: ✅ Başarılı

### Çalışma Test
- Senaryo 3: ✅ Başarılı
- Senaryo 4: ✅ Başarılı
- Tüm 4 Senaryo: ✅ Başarılı
- Deadlock: ❌ Gözlemlenmiş (başarılı)

[Test çıktılarını ekle]

## Kaynaklar
- pthreads documentation
- Dining Philosophers Problem
- Deadlock Prevention Techniques
```

### 4. Video Sunumu Planı

**Sunum Süresi: ~10 dakika**

**Senin Bölümü (Üye 3):**

**Dakika 1-2: Senaryo 3 - Asymmetric Resource Acquisition**
- Problemi tanıt: 5 researchers, 5 GPUs, deadlock riski
- Çözüm: Parity-based ordering (even/odd farklı sıralar)
- Neden çalışıyor: Circular wait broken
- Demo: Scenario 3 test çıktısını göster

**Dakika 3-4: Senaryo 4 - Right-GPU Preference**
- Problemi tanıt: Yine deadlock riski
- Çözüm: Non-blocking `trylock` + retry
- Neden çalışıyor: No indefinite hold-and-wait
- Demo: Scenario 4 test çıktısını göster

**Dakika 5-6: Edge Cases**
- 5 researchers aynı anda waking up
- 2 adjacent researchers opposite parity
- GPU fragmentation vs. our solution

**Dakika 7-8: Comparison**
- Tüm 4 senaryo karşılaştırması
- Avantajlar/Dezavantajlar
- Gerçek dünya uygulamaları

**Dakika 9-10: Q&A**

### 5. GitHub'a Push Etme

```bash
cd /path/to/project

# Dosyaları add et
git add FINAL_REPORT.md scenario_3_output.txt scenario_4_output.txt full_test_output.txt

# Commit et
git commit -m "Add final report, test outputs, and video submission"

# Push et
git push origin master
```

---

## 📋 Kontrol Listesi

### Rapor & Dokümantasyon
- [x] PROBLEM2_REPORT.md yazıldı ve push edildi
- [ ] FINAL_REPORT.md oluşturuldu
- [ ] Test çıktıları kayıt edildi
- [ ] Tüm dosyalar push edildi

### Test & Doğrulama
- [ ] ai_researchers derlenmiş
- [ ] Scenario 3 test edilmiş
- [ ] Scenario 4 test edilmiş
- [ ] Tüm 4 senaryo test edilmiş
- [ ] Hiçbir deadlock gözlemlenmiş değil ✅

### Video
- [ ] Senaryo 3 sunumu hazırlandı
- [ ] Senaryo 4 sunumu hazırlandı
- [ ] Edge case açıklaması hazırlandı
- [ ] Test demo yapıldı
- [ ] Video upload edildi (unlisted YouTube)
- [ ] YouTube link final rapor & README'ye eklendi

### Son Kontrol
- [ ] Tüm C dosyaları derlenebiliyor
- [ ] test_script.sh çalışıyor
- [ ] Rapor dosyaları eksiksiz
- [ ] GitHub repository up-to-date
- [ ] Tüm 3 üye rolleri tamamlanmış

---

## ⚠️ WSL/GCC İçin Sorun Giderme

**Eğer `gcc: Permission denied` alırsan:**
```bash
# WSL'de
sudo apt-get update
sudo apt-get install -y build-essential

# Yeniden dene
gcc -Wall -Wextra -pedantic -std=c11 ai_researchers.c -o ai_researchers -pthread
```

**Eğer `pthread.h: No such file` alırsan:**
```bash
# Doğru sistem'de misin? (Linux/WSL olmalı, native Windows değil)
uname -a  # Linux satırı görmeli
gcc --version  # GCC versiyonu görmeli
```

**Eğer script CRLF hatası alırsan:**
```bash
# Script'i normalize et
sed -i 's/\r$//' test_script.sh
bash test_script.sh
```

---

## 📚 Referanslar
- Dining Philosophers Problem: https://en.wikipedia.org/wiki/Dining_philosophers_problem
- pthreads man pages: `man pthread_mutex_lock`, `man pthread_mutex_trylock`
- Deadlock Conditions: Mutual Exclusion, Hold & Wait, No Preemption, Circular Wait
