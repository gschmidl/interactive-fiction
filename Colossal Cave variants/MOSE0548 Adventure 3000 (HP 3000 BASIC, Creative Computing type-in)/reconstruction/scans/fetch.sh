#!/bin/sh
# Fetch the ADVENTURE/3000 article pages (Creative Computing, Nov 1979, printed pp. 108-139)
# from archive.org. printed page p = leaf p+3 in the original and microfilm items, p+1 in the "better scan".
cd "$(dirname "$0")"
PAGES="108 109 110 112 114 116 118 120 122 124 126 128 130 132 134 136 138 139"
get() { # url out
  [ -s "$2" ] && { echo "have $2"; return; }
  for try in 1 2 3; do
    curl -sfL --retry 3 --max-time 1800 -o "$2.part" "$1" && mv "$2.part" "$2" && { echo "got $2 $(stat -c %s "$2")"; return; }
    echo "retry $try $2"; sleep 5
  done
  echo "FAILED $2"
}
for p in $PAGES; do
  l=$(printf %04d $((p+3))); b=$(printf %04d $((p+1)))
  get "https://archive.org/download/creativecomputing-1979-11/Creative_Computing_v05_n11_1979_November_jp2.zip/Creative_Computing_v05_n11_1979_November_jp2%2FCreative_Computing_v05_n11_1979_November_$l.jp2" "cc79_600dpi_jp2/p$p.jp2"
  get "https://archive.org/download/CreativeComputingbetterScan197911/Creative%20Computing%20%28better%20scan%29%201979-11_jp2.zip/Creative%20Computing%20%28better%20scan%29%201979-11_jp2%2FCreative%20Computing%20%28better%20scan%29%201979-11_$b.jp2" "cc79_better_jp2/p$p.jp2"
  get "https://archive.org/download/sim_creative-computing_creative-computing_1979-11_5_11/sim_creative-computing_creative-computing_1979-11_5_11_jp2.zip/sim_creative-computing_creative-computing_1979-11_5_11_jp2%2Fsim_creative-computing_creative-computing_1979-11_5_11_$l.jp2" "cc79_microfilm800_jp2/p$p.jp2"
done
# lossless originals for the program listing and sample run pages
for p in 126 128 130 132 134 136 138 139; do
  l=$(printf %04d $((p+4)))
  get "https://archive.org/download/creativecomputing-1979-11/Creative_Computing_v05_n11_1979_November.cbz/Scan-130116-$l.tif" "cc79_600dpi_tif/p$p.tif"
done
echo ALLDONE
# lossless originals for the data-file dump pages (added for the data transcription)
for p in 112 114 116 118 120 122 124; do
  l=$(printf %04d $((p+4)))
  get "https://archive.org/download/creativecomputing-1979-11/Creative_Computing_v05_n11_1979_November.cbz/Scan-130116-$l.tif" "cc79_600dpi_tif/p$p.tif"
done
echo ALLDONE2
