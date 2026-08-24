# PBAP Hörspiel-Telefonbuch – Test

Version: 0.2.24-pbap-vcards

Der Player erzeugt beim Start nach dem Bibliotheksscan automatisch vCards unter:

```
$HOME/phonebook/telecom/pb/
```

Auf dem R36S mit Benutzer `ark` ist das normalerweise:

```
/home/ark/phonebook/telecom/pb/
```

Jedes Hörspiel erhält eine Datei `<ID>.vcf`, zum Beispiel `1001.vcf`:

```
BEGIN:VCARD
VERSION:3.0
N:Hoerspielname;;;;
FN:Hoerspielname
TEL;TYPE=CELL:1001
END:VCARD
```

## Kontrolle auf dem R36S

Nach dem Start des Players:

```bash
ls -lh /home/ark/phonebook/telecom/pb/
cat /home/ark/phonebook/telecom/pb/1001.vcf
```

Im Programmlog erscheint außerdem:

```
PBAP Telefonbuch: <Anzahl> vCards in /home/ark/phonebook/telecom/pb
```

## BlueZ obexd

Für den PBAP-Test muss `obexd` mit dem BlueZ-`dummy`-Phonebook-Backend gebaut sein. Dieses Backend liest `$HOME/phonebook` als Datenquelle. Der normale Debian-Build verwendet auf dem R36S derzeit das `ebook`-Backend und muss für diesen Test durch den separat gebauten Dummy-Backend-Build ersetzt bzw. testweise gestartet werden.

Das HFP-/PulseAudio-Patch bleibt unverändert. Wird im Navi ein PBAP-Kontakt mit Telefonnummer `1001` gewählt, kommt weiterhin `ATD1001;` beim Player an und startet das Hörspiel mit der persistenten ID 1001.
