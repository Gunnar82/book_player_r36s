# PBAP Hörspiel-Telefonbuch

Der Player erzeugt nach dem Bibliotheksscan automatisch vCards unter:

```text
$HOME/phonebook/telecom/pb/
```

Auf dem R36S mit Benutzer `ark` normalerweise:

```text
/home/ark/phonebook/telecom/pb/
```

Beispiel `1001.vcf`:

```text
BEGIN:VCARD
VERSION:3.0
N:Hoerspielname;;;;
FN:Hoerspielname
TEL;TYPE=CELL:1001
END:VCARD
```

Die Nummer entspricht der persistenten Hörspiel-ID. Für PBAP muss `obexd` mit BlueZ `--with-phonebook=dummy` gebaut sein. Die vollständige Build-, Installations- und Boot-Service-Dokumentation steht in `pulse_patch/SYSTEM_PATCHES.md`.
