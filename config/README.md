# Config-bestanden per controller

Hier staat per controller het **laatste** configbestand, gedownload via `/settings` → "Configuratie bewaren" → **Downloaden** (sinds v1.16).

- Bestandsnaam: `<room>_config_<datum>.json`, bv. `Ysermonde_config_2026-11-28.json`. Oudere versie van dezelfde controller vervangen (de git-geschiedenis bewaart de vorige).
- Inhoud: alle instellingen uit het geheugen (NVS) met label, in de volgorde van `/settings`, plus room, firmwareversie, datum, MAC en IP. Wachtwoorden staan er als `***` in: veilig voor deze publieke repo.
- **Na elke wijziging in `/settings`** opnieuw downloaden en hier zetten.
- **Controller vervangen:** nieuwe controller flashen (TECHNIEK §1), via "Vakantiehuis-Setup" de Wi-Fi invullen, dan `/settings` invullen met dit bestand ernaast (wachtwoorden opnieuw ingeven). DS18B20-adressen (`ds_addr_*`) tonen welke sensor welke naam en rol had.
