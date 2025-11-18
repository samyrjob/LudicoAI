# Translation Models for VisualIA

## Current Issue with MT5 Models

The MT5 (multilingual T5) models currently in the `models/` directory are **pre-trained base models**, not fine-tuned for translation. When you try to use them for translation, they generate sentinel tokens like `<extra_id_0>` instead of actual translations.

### Why This Happens

- **MT5-base/small/large**: Pre-trained on span corruption tasks (filling in masked text)
- These models haven't been fine-tuned for translation
- They generate `<extra_id_0>` tokens which are used during pre-training, not translation

## Solution: Use MADLAD-400

**MADLAD-400** is MT5 fine-tuned specifically for translation and supports 400+ languages.

### Quick Start

Run the setup script to download and convert MADLAD-400:

```bash
bash scripts/setup_madlad_translation.sh
```

This will:
1. Install HuggingFace CLI tools
2. Download MADLAD-400 model from HuggingFace
3. Convert it to GGUF format for use with llama.cpp
4. Save it to `models/madlad400-*.gguf`

### Model Sizes

| Model | Size | Quality | Speed | Recommended For |
|-------|------|---------|-------|-----------------|
| madlad400-3b-mt | ~1.7GB | Good | Fast | Most users |
| madlad400-7b-mt | ~4.0GB | Better | Medium | Better quality needed |
| madlad400-10b-mt | ~5.5GB | Best | Slow | Maximum quality |

### Using MADLAD-400

After downloading, update your environment or code to use:

```bash
export VISUALIA_TRANSLATION_MODEL=madlad400-3b-mt
```

Or in the UI, select the MADLAD-400 model from the translation model dropdown.

## Alternative: NLLB Models

Another option is to use **NLLB-200** (No Language Left Behind) from Meta:
- Fine-tuned for translation
- Supports 200 languages
- Available in various sizes

To use NLLB, you'd need to download and convert similar to MADLAD-400.

## Current MT5 Models (Not for Translation)

The existing MT5 models in `models/` can be used for:
- Text generation tasks
- Summarization
- Question answering
- **NOT translation** (without fine-tuning)

## Supported Languages (MADLAD-400)

MADLAD-400 supports 400+ languages including:
- All major European languages (English, French, Spanish, German, Italian, etc.)
- Asian languages (Chinese, Japanese, Korean, Hindi, Arabic, etc.)
- Many low-resource languages

Full list: https://github.com/google-research/google-research/tree/master/madlad_400

## Technical Details

### Prompt Format

MADLAD-400 uses the same T5 prompt format:
```
translate English to French: Hello, how are you?
```

### Model Architecture

- Based on MT5 (multilingual T5) encoder-decoder
- Fine-tuned on 400+ language pairs
- Optimized for translation quality

## Troubleshooting

### Issue: Getting `<extra_id_0>` tokens
**Solution**: You're using MT5-base instead of MADLAD-400. Run the setup script.

### Issue: Model not found
**Solution**: Make sure the GGUF file is in `models/` directory with correct name.

### Issue: Low translation quality
**Solution**: Try a larger MADLAD model (7b or 10b instead of 3b).

## References

- [MADLAD-400 Paper](https://arxiv.org/abs/2309.04662)
- [Google Research MADLAD-400](https://github.com/google-research/google-research/tree/master/madlad_400)
- [HuggingFace MADLAD-400 Models](https://huggingface.co/models?search=madlad400)
