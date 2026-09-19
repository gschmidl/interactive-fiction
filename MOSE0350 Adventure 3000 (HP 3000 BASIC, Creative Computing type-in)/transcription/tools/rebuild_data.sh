#!/bin/sh
# Data-file dump pages: first-pass OCR with the listing's templates, nd
# matching, seeding, template training on the data pages, draft.
set -e
cd "$(dirname "$0")"
export ADV_SET=data
python geom2.py > ../.work_data/geom2.log 2>&1
python cells.py | tail -1
python ocr_data.py
python match_nd.py
python seed_data.py
rm -f ../.work_data/scores.npz
python truth.py | tail -1
python classify.py
python truth.py | tail -1
python classify.py
python draft_data.py
