#pragma once

#include <ostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace Debug {
    namespace detail {
        struct StreamState {
            StreamState(std::ostream& stream) : stream_(stream), flags_(stream.flags()), fill_(stream.fill()) {
            }
            ~StreamState() {
                stream_.flags(flags_);
                stream_.fill(fill_);
            }
            StreamState(const StreamState&) = delete;
        private:
            std::ostream &stream_;
            decltype(stream_.flags()) flags_;
            decltype(stream_.fill()) fill_;
        };

        inline unsigned char printable(unsigned char c)
        {
            if (c < 32 || c > 126) return '.';
            return c;
        }
    }

    template<size_t BPL = 16>
    void hexdump(std::ostream &stream, const unsigned char* data, size_t data_size) {
        static_assert(BPL % 2 == 0, "Bytes per line is not a multiply of two");
        detail::StreamState guard{stream};

        size_t offset = 0;
        size_t remain = data_size;
        const size_t bpl_half = BPL / 2;

        while(remain > 0) {
            const size_t bytesPerLine = remain < BPL ? remain : BPL;

            stream << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << offset << ' ';

            for(size_t i = 0; i < bytesPerLine; i++) {
                if (i != bpl_half)
                    stream.put(' ');
                else
                    stream.put('|');

                stream << std::setw(2) << static_cast<int>(data[i+offset]);
            }

            if (remain < BPL) {
                const size_t padding = (BPL - remain)*3;
                for(size_t i = 0; i < padding; i++) {
                    stream.put(' ');
                }
            }

            stream << " | ";
            for(size_t i = 0; i < bytesPerLine; i++) {
                stream << detail::printable(data[i+offset]);
            }

            stream.put('\n');

            remain -= bytesPerLine;
            offset += bytesPerLine;
        }
    }

    template<size_t BPL = 16, typename T>
    void hexdump(std::ostream &stream, const std::vector<T>& arr) {
        hexdump<BPL>(stream, arr.data(), arr.size());
    }

    template<size_t BPL = 16>
    std::string hexdump_str(const unsigned char* data, size_t data_size) {
        std::stringstream ss;

        hexdump<BPL>(ss, data, data_size);

        return ss.str();
    }
}
