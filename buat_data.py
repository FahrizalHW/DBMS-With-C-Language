import random

TARGET = int(1.3 * 1024**3)   
depan = ["Joko","Budi","Siti","Agus","Dewi","Rina","Andi","Wawan","Eko","Putri",
         "Nur","Dian","Fajar","Hendra","Ayu","Rizky","Bayu","Intan","Yusuf","Sari"]
tengah = ["Aris","Wibowo","Putra","Dwi","Tri","Eka","Nur","Adi","Surya","Cahya"]
belakang = ["Santoso","Pratama","Wijaya","Hidayat","Saputra","Lestari","Kurniawan",
            "Setiawan","Rahayu","Nugroho","Baswedan","Marjoko","Sumringah","Purwanto"]

random.seed(1)
nim = 2000000000             
written = 0
with open("dataset.csv", "w", newline="\n") as f:
    f.write("9999999,Zulkifli Wibisono Unik\n")   
    while written < TARGET:
        lines = []
        for _ in range(100000):
            nim += random.randint(1, 3)
            lines.append(f"{nim},{random.choice(depan)} {random.choice(tengah)} {random.choice(belakang)}\n")
        chunk = "".join(lines)
        f.write(chunk)
        written += len(chunk)
print("Selesai.")