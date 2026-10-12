#!/bin/sh
# Full rebuild of the listing transcription pipeline, in dependency order.
#   geom2    rows and per-row character cells          -> .work/cells_p*_c*.json
#   cells    cut every cell image                      -> .work/cells.npz, rowy.json
#   truth    align transcriptions (no glyph scores yet) -> .work/templates.npz
#   classify glyph templates, score every cell          -> .work/scores.npz, ocr.npz
#   truth    re-align using the glyph scores
#   classify rebuild templates from the better alignment
set -e
cd "$(dirname "$0")"
python geom2.py > ../.work/geom2.log 2>&1
python cells.py | tail -1
rm -f ../.work/scores.npz
python truth.py | head -1
python classify.py
python truth.py | head -1
python classify.py
python truth.py | grep -A1 MISMATCH || true
