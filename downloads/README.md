# MİRAN Gameloop bağlantı testi

[Güncel v3 ZIP paketini indir](miran_gameloop_baglanti_testi_v3.zip?raw=true).

Bu paket bağlantı tanılaması içindir; ESP oyuncu okuma ve çizimi henüz içermez.
v2, normal ADB izinleri yeterliyse UE4 dosyasının gerçek ELF mimarisini ve
GNU Build ID bilgisini de kontrol eder. Sabit hedef sürümü ölçüm gibi raporlamaz.
v3, yerel ADB bağlantısı için açık bir yeniden bağlanma düğmesi ekler; çevrimdışı
ve izin bekleyen durumları ayrı raporlar ve Gameloop'un kurulu ADB'sini önceliklendirir.

Windows'ta Python 3.10+ gerekir. ZIP'i çıkar, Gameloop ve PUBG'yi aç ve
`BASLAT_GAMELOOP_TEST.bat` dosyasını çalıştır. Uygulamada **Testi başlat**, ardından
**Raporu kaydet** seçeneklerini kullan. Ayrıntılar paketteki kullanım dosyasındadır.

[Önceki v2 paketi](miran_gameloop_baglanti_testi_v2.zip?raw=true) ·
[Önceki v1 paketi](miran_gameloop_baglanti_testi.zip?raw=true).
