# MİRAN CONTROL — 32 bit Windows uygulaması

Gönderilen C++ kodundaki fare hareketini ayarlanabilir bir Windows arayüzüne
taşır. `MiranControl32.exe` doğrudan çalışır; Python ve ek C++ çalışma zamanı
kurulumu gerektirmez. Windows 10/11 içindir; 64 bit Windows'ta da x86 uygulaması
olarak çalışır. Oyun veya emülatörün mimarisini değiştirmez.

## Kullanım

1. ZIP'i bir klasöre çıkar ve `MiranControl32.exe` dosyasını çalıştır.
2. GameLoop ve oyunu aç. Uygulamada süreç durumu görünür; gerekirse **Yenile**'ye bas.
3. Hazır ayar seç veya dikey/yatay hareket ile aralığı ayarla.
4. **Başlat**'a bas, ardından emülatörün oyun penceresini öne getir.
5. Varsayılan olarak sol ve sağ fare tuşları birlikte basılıyken hareket uygulanır.
6. **F8** veya **Acil durdur** hareketi kapatır. Yeniden başlatmak için **Başlat** gerekir.

Program her açılışta duraklatılmış başlar. Alt+Tab yapıldığında ve gereken
fare tuşları bırakıldığında hareket kesilir. GameLoop mağaza/başlatıcı penceresi
hareket için kabul edilmez. **Kaydet**, ayarları kullanıcıya ait
`%LOCALAPPDATA%\MiranControl\settings.ini` dosyasına yazar. Çalışma durumu kaydedilmez.

## Ayarlar

- Dikey hareket: 0–30 bağıl fare birimi. Pozitif değer aşağı yönlüdür.
- Yatay hareket: -15–15; negatif sola, pozitif sağa hareket eder.
- Aralık: 10–200 ms. Küçük değer daha sık hareket üretir; Windows zamanlayıcısı
  nedeniyle gerçek aralık tam sayı kadar kesin olmayabilir.
- Orijinal ayar: 5 birim aşağı / 40 ms / iki tuş birlikte.
- İki tuş seçeneği kapatılırsa yalnızca sol tuş gerekir.

Hareket birimi Windows'un bağıl fare girdisidir; ekran pikseli değildir.
Oyun hassasiyeti ve fare ayarları sonucu etkileyebilir. Ayarlar silaha göre
otomatik değişmez.

## GameLoop yolu

Varsayılan klasör:

```text
C:\Program Files\Tencent\GameLoop\Application\7.0.167.0\GameLoopBox
```

**Seç** ile değiştirilebilir; **Klasörü aç** Dosya Gezgini'nde açar. Bu alan
oyunu otomatik başlatmaz. Süreç algılama `AppVPlayer.exe` yanında `aow_exe.exe`
ve bilinen `AndroidEmulator` işlem adlarını da kontrol eder.

## Kapsam

Bu sürüm fare hareketi uygular. RAM'den oyuncu/mesafe okumaz, hedefe kilitlenmez
ve ESP çizmez. Gönderilen kodda da ana döngüde bellek okuması veya hedef seçimi
bulunmuyordu; “RAM taraması aktif” mesajı gerçek işlevi yansıtmıyordu.

Uygulama hedef sürece yalnızca kimliğini sorgulamak için sınırlı erişim ister.
Oyun belleğine yazma, `VirtualProtectEx` ve `PROCESS_ALL_ACCESS` kullanılmaz.
Windows fare girdisini reddederse uygulama durur ve ekranda hata gösterir.

## Kaynak ve doğrulama

`source` klasöründe C++ kaynakları, manifest, derleme betiği ve zamanlama testleri
bulunur. Visual Studio'nun **x86 Native Tools Command Prompt** penceresinde
`build_windows.cmd` çalıştırılarak yeniden derlenebilir.

Hazırlanan EXE'nin **PE32 / Intel i386 / Windows GUI** biçimi, manifesti ve yalnızca
Windows sistem DLL'lerine bağımlı olduğu doğrulandı. Duraklatma, tuş/ön plan
koşulları, zamanlama, gecikmiş hareketlerin birikmemesi ve değer sınırları için
mantık testleri Linux'ta geçti. Gerçek Windows arayüzü ve GameLoop üzerinde
canlı çalışma testi bu ortamda yapılmadı.
