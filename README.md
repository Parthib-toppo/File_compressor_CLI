1. First compile the C++ file:
   ```g++ -o huffman huffman.cpp```
2. Then compress an input text file:
   ```./huffman -c input.txt compressed.huff```
3. Then you can also opt to decompress the above compressed file:
   ```./huffman -d compressed.huff output.txt``` 
