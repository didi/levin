#ifndef LEVIN_SHARED_VECTOR_H
#define LEVIN_SHARED_VECTOR_H

#include <iostream>
#include "shared_base.hpp"
#include "details/vector.hpp"

namespace levin {

template <class T, class CheckFunc = levin::IntegrityChecker>
class SharedVector : public SharedBase {
public:
    typedef CustomVector<T, std::size_t>        container_type;
    typedef typename container_type::value_type value_type;
    typedef typename container_type::size_type  size_type;

    SharedVector() : SharedBase(), _size(0), _array(nullptr), _end(nullptr), _object(nullptr) {
    }
    SharedVector(const std::string &name, const std::string group = "default", const int id = 1) :
            SharedBase(name, group, id, CheckFunc()),
            _size(0),
            _array(nullptr),
            _end(nullptr),
            _object(nullptr) {
    }

    virtual int Init() override {
        int ret = _init<container_type>(_object);
        if (ret == SC_RET_OK) {
            _size = _object->size();
            _array = _object->_array();
            _end = _array + _size;
        }
        return ret;
    }

    virtual int Load() override {
        int ret = _load<container_type>(_object);
        if (ret == SC_RET_OK) {
            _size = _object->size();
            _array = _object->_array();
            _end = _array + _size;
        }
        return ret;
    }

    virtual bool Dump(const std::string &file) override;
    static bool Dump(const std::string &file, const std::vector<T> &vec, uint64_t type = 0);
    // for combination container
    static bool dump(const std::string &file, const std::vector<T> &vec, std::ofstream &fout);

    bool empty() const { return _size == 0; }
    size_t size() const { return _size; }
    const T& operator [](size_t idx) const { return _array[idx]; }
    T& operator [](size_t idx) { return _array[idx]; }
    const T& at(size_t idx) const {
        if (idx >= _size) {
            throw std::out_of_range("accessed position out of range");
        }
        return _array[idx];
    }
    const T& front() const { return _array[0]; }
    const T& back() const { return _array[_size - 1]; }
    const T* cbegin() const { return _array; }
    const T* cend() const { return _end; }
    // ret type const refrence for reminder readonly
    const T* begin() const { return _array; }
    const T* end() const { return _end; }

    void swap(SharedVector &&other) {
        std::swap(_size, other._size);
        std::swap(_array, other._array);
        std::swap(_end, other._end);
        std::swap(_object, other._object);
    }
    std::string layout() const;

protected:
    // place a copy of vector _size&_array in heap/stack for quick access
    // place a copy of vector _end in heap/stack for quick traservsal
    size_t _size = 0;
    T *_array = nullptr;
    T *_end = nullptr;
    container_type *_object = nullptr;
};

// SharedNestedVector implement as SharedVector<CustomVector<T> >
// eg. SharedNestedVector nvec("xx");
//     auto &ret = nvec[0];  // ret has type CustomVector<T>&
// template parameter SizeType customize column size type per row
// in case of nested vector has tiny vector row, SizeType specialized to uint32_t for space eficiency
// SizeType should be unsigned integral types and should specialize numeric_limits
template <class T, class SizeType = uint32_t, class CheckFunc = levin::IntegrityChecker>
class SharedNestedVector : public SharedVector<CustomVector<T, SizeType>, CheckFunc> {
public:
    typedef CustomVector<CustomVector<T, SizeType>, std::size_t> container_type;
    typedef typename container_type::value_type row_value_type;
    typedef typename row_value_type::value_type col_value_type;
    typedef typename container_type::size_type  row_size_type;
    typedef typename row_value_type::size_type  col_size_type;

    SharedNestedVector(const std::string &name, const std::string group = "default", const int id = 1) :
            SharedVector<CustomVector<T, SizeType>, CheckFunc>(name, group, id) {
    }

    virtual bool Dump(const std::string &file) override;
    static bool Dump(const std::string &file, const std::vector<std::vector<T> > &vec);
    // for combination container eg. hashmap
    static bool dump(
            const std::string &file, const std::vector<std::vector<T> > &vec, std::ofstream &fout);
    std::string layout() const;
};

template <class T, class CheckFunc>
bool SharedVector<T, CheckFunc>::Dump(const std::string &file) {
    return this->_bin2file(file, container_memsize(_object), _object);
}

template <class T, class CheckFunc>
bool SharedVector<T, CheckFunc>::Dump(
        const std::string &file, const std::vector<T> &vec, uint64_t type) {
    std::ofstream fout(file, std::ios::out | std::ios::binary);
    if (!fout.is_open()) {
        LEVIN_CWARNING_LOG("open file for write fail. file=%s", file.c_str());
        return false;
    }

    // write file header: used memory size/container type hash/container size
    size_t container_size = sizeof(container_type) + vec.size() * sizeof(value_type);
    size_t type_hash = (type == 0 ? typeid(container_type).hash_code() : type);
    SharedFileHeader header = {container_size, type_hash, makeFlags(SharedBase::SC_VERSION)};
    fout.write((const char*)&header, sizeof(header));
    if (!fout) {
        LEVIN_CWARNING_LOG("write file fail. file=%s", file.c_str());
        return false;
    }
    return SharedVector<value_type>::dump(file, vec, fout);
}

template <class T, class CheckFunc>
bool SharedVector<T, CheckFunc>::dump(
        const std::string &file, const std::vector<T> &vec, std::ofstream &fout) {
    static const size_t size_limit = std::numeric_limits<size_type>::max();
    static const size_t fix_offset = sizeof(container_type);
    // write customvector header: size/offset
    size_t array_size = vec.size();
    if (fix_offset > size_limit || array_size > size_limit) {
        LEVIN_CWARNING_LOG("size/offset exceed size_type(%s) numeric limit, write file fail. "
                "file=%s", demangle(typeid(size_type).name()).c_str(), file.c_str());
        return false;
    }
    std::vector<size_type> headers = {
        static_cast<size_type>(array_size), static_cast<size_type>(fix_offset)};
    fout.write((const char*)headers.data(), sizeof(size_type) * headers.size());
    if (!fout) {
        LEVIN_CWARNING_LOG("write file fail. file=%s", file.c_str());
        return false;
    }

    // write array
    fout.write((const char*)vec.data(), sizeof(value_type) * array_size);
    if (!fout) {
        LEVIN_CWARNING_LOG("write file fail. file=%s", file.c_str());
        return false;
    }
    return true;
}

template <class T, class CheckFunc>
std::string SharedVector<T, CheckFunc>::layout() const {
    std::stringstream ss;
    ss << "SharedVector this=[" << (void*)this << "]";
    if (_info->_meta != nullptr) {
        ss << std::endl << *_info->_meta;
    }
    if (_object != nullptr) {
        ss << std::endl << _object->layout();
    }
    return ss.str();
}

template <class T, class SizeType, class CheckFunc>
bool SharedNestedVector<T, SizeType, CheckFunc>::Dump(const std::string &file) {
    return this->_bin2file(file, container_memsize(this->_object), this->_object);
}

template <class T, class SizeType, class CheckFunc>
bool SharedNestedVector<T, SizeType, CheckFunc>::Dump(
        const std::string &file, const std::vector<std::vector<T> > &vec) {
    std::ofstream fout(file, std::ios::out | std::ios::binary);
    if (!fout.is_open()) {
        LEVIN_CWARNING_LOG("open file for write fail. file=%s", file.c_str());
        return false;
    }
    // write file header: used memory size(meta size + container size)/container type hash
    size_t container_size = sizeof(container_type);
    for (auto &row : vec) {
        container_size += sizeof(row_value_type);
        container_size += row.size() * sizeof(col_value_type);
    }
    size_t type_hash = typeid(container_type).hash_code();
    SharedFileHeader header = {container_size, type_hash, makeFlags(SharedBase::SC_VERSION)};
    fout.write((const char*)&header, sizeof(header));
    if (!fout) {
        LEVIN_CWARNING_LOG("write file fail. file=%s", file.c_str());
        return false;
    }
    return SharedNestedVector<col_value_type, SizeType>::dump(file, vec, fout);
}

template <class T, class SizeType, class CheckFunc>
bool SharedNestedVector<T, SizeType, CheckFunc>::dump(
        const std::string &file, const std::vector<std::vector<T> > &vec, std::ofstream &fout) {
    static const size_t row_size_limit = std::numeric_limits<row_size_type>::max();
    static const size_t col_size_limit = std::numeric_limits<col_size_type>::max();
    static const size_t fix_row_offset = sizeof(container_type);
    static const size_t fix_col_offset = sizeof(row_value_type);

    // write customvector header: size/offset
    size_t row_size = vec.size();
    if (fix_row_offset > row_size_limit || row_size > row_size_limit) {
        LEVIN_CWARNING_LOG("column size/offset exceed size_type(%s) numeric limit, "
                "write file fail. file=%s",
                demangle(typeid(row_size_type).name()).c_str(), file.c_str());
        return false;
    }
    std::vector<row_size_type> row_headers = {
        static_cast<row_size_type>(row_size), static_cast<row_size_type>(fix_row_offset)};
    fout.write((const char*)row_headers.data(), sizeof(row_size_type) * row_headers.size());
    if (!fout) {
        LEVIN_CWARNING_LOG("write file fail. file=%s", file.c_str());
        return false;
    }

    // write column header
    size_t cur_offset = fix_col_offset * row_size;
    for (size_t i = 0; i < row_size; ++i) {
        size_t col_size = vec[i].size();
        if (cur_offset > col_size_limit || col_size > col_size_limit) {
            LEVIN_CWARNING_LOG("column size/offset exceed size_type(%s) numeric limit, "
                    "write file fail. file=%s",
                    demangle(typeid(col_size_type).name()).c_str(), file.c_str());
            return false;
        }
        std::vector<col_size_type> col_headers = {
            static_cast<col_size_type>(col_size), static_cast<col_size_type>(cur_offset)};
        fout.write((const char*)col_headers.data(), sizeof(col_size_type) * col_headers.size());
        if (!fout) {
            LEVIN_CWARNING_LOG("write file fail. file=%s", file.c_str());
            return false;
        }
        cur_offset += col_size * sizeof(col_value_type) - fix_col_offset;
    }
    // write column array
    for (size_t i = 0; i < row_size; ++i) {
        fout.write((const char*)vec[i].data(), sizeof(col_value_type) * vec[i].size());
        if (!fout) {
            LEVIN_CWARNING_LOG("write file fail. file=%s", file.c_str());
            return false;
        }
    }
    return true;
}

template <class T, class SizeType, class CheckFunc>
std::string SharedNestedVector<T, SizeType, CheckFunc>::layout() const {
    std::stringstream ss;
    ss << "SharedNestedVector this=[" << (void*)this << "]";
    if (this->_info->_meta != nullptr) {
        ss << std::endl << *this->_info->_meta;
    }
    if (this->_object != nullptr) {
        ss << std::endl << container_layout(this->_object);
    }
    return ss.str();
}

}  // namespace levin

#endif  // LEVIN_SHARED_VECTOR_H
