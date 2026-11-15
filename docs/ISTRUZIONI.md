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
   - `ToxicRig-Windows` → contiene `ToxicRig.vst3` (+ `ToxicRig.exe` standalone)
   - `ToxicRig-macOS` → `ToxicRig.vst3`, `ToxicRig.component` (AU), `ToxicRig.app`
   - `TestReport-...` → il report dei test fatto direttamente su Windows/Mac

Ogni volta che faremo modifiche basta `git add . && git commit -m "..." && git push`.

## B. Installare su Windows (FL Studio)

1. Estrai `ToxicRig-Windows.zip`.
2. Copia la cartella **`ToxicRig.vst3`** (è una cartella, copiala intera) in:
   `C:\Program Files\Common Files\VST3\`
   (Windows chiede i permessi di amministratore: conferma.)
3. FL Studio → **Options → Manage plugins** → **Find more plugins** (o "Start scan").
   Cerca "ToxicRig" nella lista e metti la stellina per averlo tra i preferiti.
4. Nel Mixer: seleziona l'insert dove arriva la chitarra → slot effetti → **ToxicRig**.

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

1. Copia `ToxicRig.component` in `~/Library/Audio/Plug-Ins/Components/`
   e `ToxicRig.vst3` in `~/Library/Audio/Plug-Ins/VST3/`.
2. Il plugin non è ancora firmato con un certificato Apple (costa 99 $/anno, serve per
   vendere). Per provarlo, nel Terminale:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/ToxicRig.component
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/ToxicRig.vst3
   ```

## E. Come si usa

- **Striscia in alto** = la catena del suono, da sinistra a destra.
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
