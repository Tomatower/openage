// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#pragma once

#include "packet.h"
#include "wiremanager.h"

#include "../log/log.h"

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
    SerializerStream(openage::log::NamedLogSource &log) :
        is_write_{true},
        logsink{log}
	{}


    /** \brief Write data to the end of the stream
     *
     * \param data const int8_t* the data to be written
     * \param length size_t the number of bytes to be written
     * \return SerializerStream& *this
     *
     */
    SerializerStream &write(const uint8_t *data, size_t length);

    /** \brief Read data from the beginning of the stream
     *
     * \param data int8_t*
     * \param length size_t
     * \return SerializerStream&
     *
     */
    SerializerStream &read(uint8_t *data, size_t length);

    ///@TODO add more basic datatypes - maybe templated?

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
	void set_data(int8_t *start, size_t count);

    /** \brief Get the serialized data from the package again.
     *
     * This buffer can be immediately fed back into the @see set_data method
     *
     * \param buffer std::vector<int8_t>&
     * \return void
     *
     */
    size_t get_data(std::vector<int8_t> &buffer) const;
	size_t get_data(int8_t *start, size_t max_len);

    /** \brief Set if the stream is currently configured for reading or writing
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

    SerializerStream &on_wire(int32_t *data);
    SerializerStream &on_wire(std::string *data);
    SerializerStream &on_wire(uint16_t *data);

    template <typename _type>
    SerializerStream &on_wire(_type *data) {
        if (this->is_write()) {
            this->write((uint8_t *)data, sizeof(*data));
        } else {
            this->read((uint8_t *)data, sizeof(*data));
        }
        return *this;
    }


    SerializerStream &on_wire(Packet *data);
    SerializerStream &on_wire(Packet::input *data);
    SerializerStream &on_wire(Packet::nyanchange *data);
    SerializerStream &on_wire(Packet::object_state *data);
    SerializerStream &on_wire(Packet::trajectory_element *data);

	template <typename _type>
    SerializerStream &on_wire(WireManager::reliable<_type> *data) {
    	this->on_wire(&data->remote_frame);
		this->deque_on_wire(&data->data);

		return *this;
	}

    SerializerStream &map_on_wire(std::unordered_map<int32_t, int32_t> *data);

    template <typename _Base>
    SerializerStream &deque_on_wire(std::deque<_Base> *data) {
        int16_t cnt = data->size();
        this->on_wire(&cnt);

        //logsink.log(INFO << "DEQUE: " << is_write() << " cnt " << cnt);

        if (this->is_write()) {
            for (auto it = data->begin(); it != data->end(); ++it) {
                this->on_wire(&*it);
            }
        } else {
            for (int i = 0; i < cnt; ++i) {
                _Base p;
                this->on_wire(&p);
                data->push_back(p);
            }
        }
        return *this;
    }


    /** \brief write data, but keep the beginning of the write operation as return.
     *
     * DO NOT USE THIS except you know exactly what you are doing.
     * This can be used for example to track the count parameter when inserting a unknown number of objects into the
     * stream, but should be encoded that the number preceeds the objects.
     *
     * \param data int8_t* Data to be written
     * \param size size_t Size of the data to be written
     * \return int8_t* Pointer to the beginning of the data written
     *
     */
    uint8_t *write_observable(uint8_t *data, size_t size);

private:
    std::deque<uint8_t> buffer;
    bool is_write_;
	openage::log::NamedLogSource &logsink;
};

}
} // openage::network
