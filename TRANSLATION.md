# Translation Guide

VisualIA supports **live real-time translation** of transcribed speech.

## How It Works

1. **Audio** → Whisper STT → **Text** (original language)
2. **Text** → LibreTranslate API → **Translation** (target language)
3. Display both original and translated text in overlay

## Free Translation Options

### 1. LibreTranslate (Default, Recommended)
- **Free**: Yes, public API available
- **Privacy**: Can self-host for 100% privacy
- **Speed**: ~200-500ms per request
- **Limit**: Rate-limited on public instance
- **Quality**: Good for most languages

**Self-hosting:**
```bash
# Using Docker
docker run -ti --rm -p 5000:5000 libretranslate/libretranslate

# Then update renderer.js to use localhost:5000
```

### 2. Google Translate (Unofficial API)
Requires third-party library. Better quality but may have rate limits.

### 3. Argos Translate (Offline)
Fully offline, runs locally. Slower but completely private.

## Supported Translation Languages

The UI supports translation to these languages:
- 🇬🇧 English
- 🇫🇷 French
- 🇪🇸 Spanish
- 🇩🇪 German
- 🇮🇹 Italian
- 🇵🇹 Portuguese
- 🇳🇱 Dutch
- 🇵🇱 Polish
- 🇷🇺 Russian
- 🇨🇳 Chinese (Simplified)
- 🇯🇵 Japanese
- 🇰🇷 Korean
- 🇸🇦 Arabic
- 🇮🇳 Hindi
- 🇹🇷 Turkish

## Usage

### Via UI (Recommended)
1. Start VisualIA
2. In top-right panel, select "Translate To" language
3. Speak in any language
4. See original + translation in subtitle box

### Disable Translation
Select "⊗ None" in the "Translate To" dropdown

## Performance

### Translation Speed
- **Cached**: Instant (if same text already translated)
- **API Call**: 200-500ms average
- **Total Latency**: ~3.5 seconds (3s Whisper + 0.5s translation)

### Cache
- Translations are cached in memory
- Max 100 cached translations
- Cleared on app restart

### Optimization Tips
1. **Use self-hosted LibreTranslate** for faster response
2. **Enable caching** (already done)
3. **Reduce Whisper chunk size** for faster transcription
4. **Use smaller Whisper model** (base vs large)

## Examples

### Speak French → Translate to English
```
Source Language: 🇫🇷 French (or Auto-Detect)
Translate To: 🇬🇧 English

You say: "Bonjour, comment allez-vous?"
Display:
  ORIGINAL: Bonjour, comment allez-vous?
  TRANSLATION: Hello, how are you?
```

### Speak English → Translate to Japanese
```
Source Language: 🇬🇧 English (or Auto-Detect)
Translate To: 🇯🇵 Japanese

You say: "Good morning, how can I help you?"
Display:
  ORIGINAL: Good morning, how can I help you?
  TRANSLATION: おはようございます、どのようにお手伝いできますか？
```

## Self-Hosting LibreTranslate

For unlimited, fast, private translation:

```bash
# Clone repo
git clone https://github.com/LibreTranslate/LibreTranslate
cd LibreTranslate

# Install
pip install -e .

# Download language models (only what you need)
./install_models.sh

# Run server
libretranslate --host 0.0.0.0 --port 5000
```

Then update `frontend/src/renderer.js`:
```javascript
const response = await fetch('http://localhost:5000/translate', {
    // ... same config
});
```

## Alternative Translation APIs

### DeepL (Better Quality)
```javascript
// In renderer.js, replace translateText function
const response = await fetch('https://api-free.deepl.com/v2/translate', {
    method: 'POST',
    headers: {
        'Authorization': 'DeepL-Auth-Key YOUR_API_KEY',
        'Content-Type': 'application/json'
    },
    body: JSON.stringify({
        text: [text],
        target_lang: targetLang.toUpperCase()
    })
});
```

### Google Cloud Translation API
Best quality, requires API key and billing.

### Offline (Argos Translate)
Can integrate with Python backend for fully offline translation.

## Troubleshooting

### Translation shows "[Translation unavailable]"
- **Cause**: LibreTranslate API rate limit or network error
- **Solution**: Wait a few seconds or self-host LibreTranslate

### Translation is slow
- **Cause**: Network latency to public API
- **Solution**: Self-host LibreTranslate locally

### Translation quality is poor
- **Cause**: LibreTranslate uses neural models but not as advanced as DeepL/Google
- **Solution**:
  - Switch to DeepL API (requires key)
  - Self-host with better models
  - Use Google Translate API

### Certain languages don't work
- **Cause**: Not all language pairs are equally supported
- **Solution**: Check LibreTranslate supported pairs or use different API

## Future Enhancements

- [ ] Offline translation with local models
- [ ] Multiple translation targets simultaneously
- [ ] Translation history/logging
- [ ] Pronunciation guides for translations
- [ ] Voice output for translations (TTS)
- [ ] Custom translation API endpoints
- [ ] Translation confidence scores
