#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <string>

using namespace std;

// Huffman Tree Node
struct Node {
    char ch;
    int freq;
    Node *left, *right;

    Node(char c, int f) : ch(c), freq(f), left(nullptr), right(nullptr) {}
    Node(char c, int f, Node *l, Node *r) : ch(c), freq(f), left(l), right(r) {}
};

// Comparison object for priority queue (min-heap based on frequency)
struct Compare {
    bool operator()(Node *left, Node *right) {
        return left->freq > right->freq;
    }
};

// Function to build frequency map from the input text
unordered_map<char, int> buildFrequencyMap(const string &text) {
    unordered_map<char, int> freqMap;
    for (char ch : text) {
        freqMap[ch]++;
    }
    return freqMap;
}

// Function to build Huffman Tree from frequency map
Node* buildHuffmanTree(const unordered_map<char, int> &freqMap) {
    priority_queue<Node*, vector<Node*>, Compare> minHeap;

    // Create leaf nodes for each character and add to minHeap
    for (const auto &pair : freqMap) {
        Node *node = new Node(pair.first, pair.second);
        minHeap.push(node);
    }

    // Build the tree
    while (minHeap.size() > 1) {
        Node *left = minHeap.top(); minHeap.pop();
        Node *right = minHeap.top(); minHeap.pop();

        Node *merged = new Node('\0', left->freq + right->freq, left, right);
        minHeap.push(merged);
    }

    // Return the root of the tree
    return minHeap.top();
}

// Function to generate Huffman codes from the Huffman tree
void generateCodes(Node *root, const string &str, unordered_map<char, string> &huffmanCode) {
    if (root == nullptr)
        return;

    // If it's a leaf node, assign the code
    if (!root->left && !root->right) {
        huffmanCode[root->ch] = str;
    }

    generateCodes(root->left, str + "0", huffmanCode);
    generateCodes(root->right, str + "1", huffmanCode);
}

// Function to encode the text using the Huffman codes
string encodeText(const string &text, const unordered_map<char, string> &huffmanCode) {
    string encodedText;
    for (char ch : text) {
        encodedText += huffmanCode.at(ch);
    }
    return encodedText;
}

// Function to write the frequency map to the compressed file
void writeFrequencyMap(ofstream &outFile, const unordered_map<char, int> &freqMap) {
    uint16_t mapSize = freqMap.size();
    outFile.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));

    for (const auto &pair : freqMap) {
        char ch = pair.first;
        uint32_t freq = pair.second;

        outFile.write(&ch, sizeof(ch));
        outFile.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
    }
}

// Function to read the frequency map from the compressed file
unordered_map<char, int> readFrequencyMap(ifstream &inFile) {
    unordered_map<char, int> freqMap;

    uint16_t mapSize;
    inFile.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));

    for (uint16_t i = 0; i < mapSize; ++i) {
        char ch;
        uint32_t freq;
        inFile.read(&ch, sizeof(ch));
        inFile.read(reinterpret_cast<char*>(&freq), sizeof(freq));
        freqMap[ch] = freq;
    }

    return freqMap;
}

// Function to write the encoded data to the compressed file
void writeEncodedData(ofstream &outFile, const string &encodedText) {
    // Calculate padding bits
    uint8_t padding = 8 - (encodedText.length() % 8);
    if (padding == 8)
        padding = 0;

    outFile.write(reinterpret_cast<const char*>(&padding), sizeof(padding));

    // Append '0's to the encoded text to make it byte-aligned
    string paddedEncodedText = encodedText;
    for (int i = 0; i < padding; ++i) {
        paddedEncodedText += '0';
    }

    // Write the encoded data as bytes
    for (size_t i = 0; i < paddedEncodedText.length(); i += 8) {
        string byteString = paddedEncodedText.substr(i, 8);
        uint8_t byte = bitset<8>(byteString).to_ulong();
        outFile.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
    }
}

// Function to read the encoded data from the compressed file
string readEncodedData(ifstream &inFile, uint8_t &padding) {
    inFile.read(reinterpret_cast<char*>(&padding), sizeof(padding));

    // Read the rest of the file as bytes
    string encodedText;
    char byte;
    while (inFile.read(&byte, sizeof(byte))) {
        bitset<8> bits(static_cast<unsigned char>(byte));
        encodedText += bits.to_string();
    }

    // Remove the padding bits from the end
    if (padding > 0 && !encodedText.empty()) {
        encodedText.erase(encodedText.end() - padding, encodedText.end());
    }

    return encodedText;
}

// Function to free the Huffman tree from memory
void freeTree(Node *root) {
    if (root == nullptr)
        return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

// Compression function
void compress(const string &inputFileName, const string &outputFileName) {
    // Read input text
    ifstream inFile(inputFileName, ios::binary);
    if (!inFile.is_open()) {
        cerr << "Error opening input file.\n";
        return;
    }

    string text((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    // Build frequency map
    unordered_map<char, int> freqMap = buildFrequencyMap(text);

    // Build Huffman tree
    Node *root = buildHuffmanTree(freqMap);

    // Generate codes
    unordered_map<char, string> huffmanCode;
    generateCodes(root, "", huffmanCode);

    // Encode text
    string encodedText = encodeText(text, huffmanCode);

    // Write compressed file
    ofstream outFile(outputFileName, ios::binary);
    if (!outFile.is_open()) {
        cerr << "Error opening output file.\n";
        return;
    }

    // Write frequency map
    writeFrequencyMap(outFile, freqMap);

    // Write encoded data
    writeEncodedData(outFile, encodedText);

    outFile.close();

    // Free memory
    freeTree(root);

    cout << "File compressed successfully. Output file: " << outputFileName << endl;
}

// Decompression function
void decompress(const string &inputFileName, const string &outputFileName) {
    // Open compressed file
    ifstream inFile(inputFileName, ios::binary);
    if (!inFile.is_open()) {
        cerr << "Error opening input file.\n";
        return;
    }

    // Read frequency map
    unordered_map<char, int> freqMap = readFrequencyMap(inFile);

    // Rebuild Huffman tree
    Node *root = buildHuffmanTree(freqMap);

    // Read encoded data
    uint8_t padding;
    string encodedText = readEncodedData(inFile, padding);
    inFile.close();

    // Decode the data
    ofstream outFile(outputFileName, ios::binary);
    if (!outFile.is_open()) {
        cerr << "Error opening output file.\n";
        return;
    }

    Node *current = root;
    for (size_t i = 0; i < encodedText.length(); ++i) {
        if (encodedText[i] == '0') {
            current = current->left;
        } else {
            current = current->right;
        }

        // If leaf node
        if (!current->left && !current->right) {
            outFile.put(current->ch);
            current = root;
        }
    }

    outFile.close();

    // Free memory
    freeTree(root);

    cout << "File decompressed successfully. Output file: " << outputFileName << endl;
}

// Main function for the CLI
int main(int argc, char *argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " -c|-d <input_file> <output_file>\n";
        cerr << "Options:\n";
        cerr << "  -c    Compress the input file\n";
        cerr << "  -d    Decompress the input file\n";
        return 1;
    }

    string mode = argv[1];
    string inputFileName = argv[2];
    string outputFileName = argv[3];

    if (mode == "-c") {
        compress(inputFileName, outputFileName);
    } else if (mode == "-d") {
        decompress(inputFileName, outputFileName);
    } else {
        cerr << "Invalid option. Use -c to compress or -d to decompress.\n";
        return 1;
    }

    return 0;
}
