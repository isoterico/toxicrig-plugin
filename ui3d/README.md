# ToxicRig - palco 3D in tempo reale

- amp_british30.py: modello Blender (python3 amp_british30.py -- gltf)
- textures.py: texture procedurali
- src/: scena three.js; src/pedals.js = parametri di plugin/Params.cpp
- Build: npm i three esbuild && npx esbuild src/main.js --bundle --minify --format=iife --outfile=public/app.js
