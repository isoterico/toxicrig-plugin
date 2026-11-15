# ToxicRig — istruzioni passo passo

## A. Far compilare il plugin da GitHub (una volta sola, ~15 minuti)

Perché: io lavoro su un server Linux e non posso produrre direttamente i file per
Windows e Mac. GitHub Actions accende gratis un PC Windows e un Mac, compila e ti
lascia i file pronti da scaricare.

1. Vai su https://github.com/new
   - Repository name: `toxicrig`
   - Seleziona **Private**
   - NON spuntare "Add a README" → **Create repository**
2. Estrai lo zip `ToxicRig_sorgenti.zip` in una cartella, per esempio `C:\Dev\toxicrig`.
3. Apri il **Prompt dei comandi** in quella cartella (in Esplora file: clic sulla barra
   dell'indirizzo, scrivi `cmd`, Invio) e incolla, sostituendo TUO-UTENTE:
   ```
   git init
   git add .
   git commit -m "ToxicRig v0.1"
   git branch -M main
   git remote add origin https://github.com/TUO-UTENTE/toxicrig.git
   git push -u origin main
   ```
4. Su GitHub apri il repository → scheda **Actions**. Parte da sola la build
   "Build ToxicRig" (pallino giallo). Dura ~10-15 minuti la prima volta.
5. Quando diventa verde, cliccaci → in fondo alla pagina, sezione **Artifacts**:
   - **`ToxicRig-Windows-INSTALLER`** → dentro c'è `ToxicRig-Setup-0.2.0.exe`
   - **`ToxicRig-macOS-INSTALLER`** → dentro c'è `ToxicRig-Installer-0.2.0.pkg`
   - `...-file-sciolti` → gli stessi file senza installer (servono solo per debug)
   - `TestReport-...` → il report dei test eseguito direttamente su Windows e Mac

Ogni volta che faremo modifiche basta `git add . && git commit -m "..." && git push`:
GitHub ricompila tutto e crea i nuovi installer.

## B. Installare su Windows

1. Estrai lo zip e fai doppio clic su **`ToxicRig-Setup-0.2.0.exe`**.
2. Windows potrebbe mostrare "Windows ha protetto il PC" (SmartScreen): succede con
   tutti i programmi non ancora firmati con un certificato digitale. Clicca
   **Ulteriori informazioni → Esegui comunque**. Per la vendita compreremo il
   certificato di firma, così l'avviso sparisce.
3. Segui la procedura: lingua → licenza → componenti (VST3 sempre, Standalone se vuoi
   suonare anche senza DAW) → **Installa** → **Fine**.
   Il plugin finisce in `C:\Program Files\Common Files\VST3\`, la cartella dove FL Studio
   e tutte le DAW cercano i VST3.
4. FL Studio → **Options → Manage plugins** → **Find more plugins**. Cerca "ToxicRig"
   e mettilo tra i preferiti con la stellina.
5. Per disinstallarlo: Impostazioni di Windows → App → App installate → ToxicRig.

## C. Focusrite + FL Studio senza ritardo

1. Collega la Telecaster all'ingresso **INST** (tasto INST acceso) della Focusrite.
   Perché: l'ingresso strumento ha impedenza alta (~1 MΩ); quello linea caricherebbe
   il pickup e ti spegnerebbe gli acuti prima ancora del plugin.
2. Guadagno: suona forte con il pickup al ponte; l'anello della Focusrite deve restare
   verde, al massimo giallo per un istante. Mai rosso (il clipping del convertitore
   non si toglie più dopo).
3. FL Studio → **Options → Audio settings** → Device: **Focusrite USB ASIO**.
   Perché ASIO: parla direttamente col driver della scheda, saltando il mixer di
   Windows che aggiunge decine di millisecondi.
4. Clicca **Show ASIO panel** → Buffer Size **128 samples** (prova 64 se il PC regge
   senza crepitii; 256 se crepita). Sample rate 48 kHz.
   Con 128 a 48 kHz: ~2,7 ms in + 2,7 ms out + 1,4 ms del plugin ≈ 7-8 ms totali,
   sotto la soglia in cui la mano "sente" il ritardo (~10 ms).
5. Nel Mixer, sull'insert della chitarra, scegli come input la Focusrite (Input 1).
   Arma l'insert per sentirlo in tempo reale.
6. Nel plugin regola **Input** finché il meter d'ingresso arriva a circa metà (−12 dB)
   quando suoni forte. Il plugin è tarato su questo livello: con l'input giusto i
   preset suonano come li ho pensati.

Se senti crepitii: alza il buffer, oppure metti Oversampling su **2x**.

## D. Mac (Apple Silicon)

1. Doppio clic su **`ToxicRig-Installer-0.2.0.pkg`**.
2. Se macOS dice che non può verificare lo sviluppatore: **tasto destro sul file → Apri
   → Apri** (oppure Impostazioni di Sistema → Privacy e sicurezza → "Apri comunque").
   Anche qui serve il certificato Apple (99 $/anno) per la vendita.
3. Segui la procedura: installa VST3, AU e app (con "Personalizza" scegli cosa).

## E. Come si usa

- **Stile grafico**: ingranaggio ⚙ in alto → Stile grafico (Toxic Lab, Studio Noir,
  Schematic). Stesse funzioni, cambia solo l'aspetto. Da lì scegli anche la dimensione
  della finestra; puoi anche trascinare l'angolo in basso a destra.
- **Toxic Lab**: le manopole dell'ampli si girano direttamente sull'ampli. A destra
  scegli Ampli A/B, il modello (< >), il canale e il mixer A/B; "CASSA E MICROFONO"
  apre la cassa. In basso i pedali: clic = si apre in grande, trascina = cambi ordine,
  clic sul pulsante a pedale = acceso/spento.
- **Studio Noir / Schematic**: striscia in alto con la catena del suono:
  - Clic su un blocco = lo modifichi sotto.
  - Clic sul **LED** di un pedale = acceso/spento.
  - **Trascina** un pedale per cambiare ordine, o spostalo oltre AMP/CAB per
    metterlo nel loop effetti (dopo l'ampli). Delay e riverbero stanno solo nel loop.
- **AMP**: ampli A e B. "Ampli B ON" li mette in parallelo (A a sinistra, B a destra,
  Width allarga). Sul British, Presence e Resonance diventano **Cut** e **Depth**.
- **CAB**: tipo di cassa o la tua IR (.wav), tipo di microfono, e trascina il pallino
  verde sul cono: orizzontale = centro↔bordo, verticale = distanza.
- **A / B**: due versioni del suono da confrontare al volo.
- **Oversampling**: 4x di base. 8x = ancora meno aliasing, ma il doppio di CPU.
- Doppio clic su una manopola = torna al valore di default.
