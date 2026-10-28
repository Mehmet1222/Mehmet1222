# MİRAN indirmeleri

[MİRAN CONTROL 32 bit — EXE ve kaynak kod ZIP](MiranControl32.zip?raw=true) ·
[Yalnızca 32 bit EXE](MiranControl32.exe?raw=true)

GameLoop için arayüzlü fare hareketi uygulaması: dikey/yatay ayarlar, aralık,
hazır profiller, kayıt ve F8 ile acil durdurma. Python gerekmez. Verilen GameLoop
klasörü varsayılan yoldur. RAM hedef taraması, aimbot veya ESP içermez.
32 bit Windows GUI derlemesi ve mantık testleri doğrulandı; Windows/GameLoop
üzerinde canlı çalışma testi bu ortamda yapılmadı.
[Kullanım ve kaynak](../miran_control32/KULLANIM.md).

## Önceki GameLoop bağlantı testleri

[Güncel v4 ZIP paketini indir](miran_gameloop_baglanti_testi_v4.zip?raw=true).

Bu paket bağlantı tanılaması içindir; ESP oyuncu okuma ve çizimi henüz içermez.
v2, normal ADB izinleri yeterliyse UE4 dosyasının gerçek ELF mimarisini ve
GNU Build ID bilgisini de kontrol eder. Sabit hedef sürümü ölçüm gibi raporlamaz.
v3, yerel ADB bağlantısı için açık bir yeniden bağlanma düğmesi ekler; çevrimdışı
ve izin bekleyen durumları ayrı raporlar ve Gameloop'un kurulu ADB'sini önceliklendirir.
v4, eski ADB için sınırlı metin aktarımı ve ayrı dosya okuma hata kodları ekler.
Gönderilen offset adayları ayrıca inceleme kaydı olarak saklanır; doğrulanmış
oyun adresleri olarak kullanılmaz. Linux'ta 76 otomatik test ve Tkinter arayüz
kontrolü geçti; gerçek Windows/GameLoop testi bu ortamda yapılmadı.

Windows'ta Python 3.10+ gerekir. ZIP'i çıkar, Gameloop ve PUBG'yi aç ve
`BASLAT_GAMELOOP_TEST.bat` dosyasını çalıştır. Uygulamada **Testi başlat**, ardından
**Raporu kaydet** seçeneklerini kullan. Ayrıntılar paketteki kullanım dosyasındadır.

[Önceki v3 paketi](miran_gameloop_baglanti_testi_v3.zip?raw=true) ·
[Önceki v2 paketi](miran_gameloop_baglanti_testi_v2.zip?raw=true) ·
[Önceki v1 paketi](miran_gameloop_baglanti_testi.zip?raw=true).
