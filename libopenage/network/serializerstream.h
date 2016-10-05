// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#pragma once

#include <deque>
#include <string>
#include <vector>

namespace openage {
namespace network {

/** \brief Tool to serialize packets into FIFO byte streams
 *
 * Serializing stuff FIFO-Style has the advantage, that exactly the same code can be used to read and write.
 * This helps avoiding serialization errors, even in complex datatypes.
 *
 * Whenever data is written to the stream, it is done in the end, when it is read it comes from the beginning.
 *
 * To serialize complex structs, the @see on_wire methods can be used, like this:
 *
 * <pre>
 * struct test { int a, int b };
 *
 * void on_wire(SerializerStream &ss, test &data) {
 *     ss.on_wire(data.a)
 *       .on_wire(data.b);
 * }
 * </pre>
 *
 * When this keeps the same order of execution when reading or writing.
 *
 */
class SerializerStream {
public:

    /** \brief C'tor
     */
    SerializerStream() :
        is_write_{true} {}


    /** \brief Write data to the end of the stream
     *
     * \param data const int8_t* the data to be written
     * \param length size_t the number of bytes to be written
     * \return SerializerStream& *this
     *
     */
    SerializerStream &write(const int8_t *data, size_t length);

    /** \brief Read data from the beginning of the stream
     *
     *
     * \param data int8_t*
     * \param length size_t
     * \return SerializerStream&
     *
     */
    SerializerStream &read(int8_t *data, size_t length);

    ///@TODO add more basic datatypes - maybe templated?

    /** \brief write / read int data
     *
     * \param data int*
     * \return SerializerStream&
     *
     */
    SerializerStream &on_wire(int *data);


    /** \brief write / read string data
     *
     * \param data std::string*
     * \return SerializerStream&
     *
     */
    SerializerStream &on_wire(std::string *data);

    /** \brief Drop all saved data.
     *
     * \return void
     *
     */
    void clear();

    /** \brief Store raw data to be deserialized.
     *
     * \param buffer const std::vector<int8_t>& the data container. buffer.size is used to determine the buffersize
     * \return void
     *
     */
    void set_data(const std::vector<int8_t> &buffer);

    /** \brief Get the serialized data from the package again.
     *
     * This buffer can be immediately fed back into the @see set_data method
     *
     * \param buffer std::vector<int8_t>&
     * \return void
     *
     */
    void get_data(std::vector<int8_t> &buffer) const;

    /** \brief Set if the stream is currently configured for reading or writing
     *
     *
     * \param write bool true => write data to this stream. false => read data into objects
     * \return void
     *
     */
    void set_write_mode(bool write);

    /** \brief Get the write-state of the stream
     *
     * \return bool
     *
     */
    bool is_write() const;

    /** \brief get the number of bytes stored
     *
     * \return size_t
     *
     */
    size_t size();
private:
    std::deque<int8_t> buffer;
    bool is_write_;
};

}
} // openage::network
