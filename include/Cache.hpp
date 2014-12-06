#ifndef _CACHE_HPP_
#define _CACHE_HPP_

#include <vector>
#include <map>

/*
 * Cache class template. Key - key to refer to cache record. T - cache record type
 */
template<typename Key, typename T>
class Cache {
protected:
    bool m_isValid;
    std::vector<T> m_cached_values;
    std::map<Key, T> m_cached_values_map;

public:
    // create empty cache. validness is undefined
    Cache() {
        m_cached_values.clear();
        m_cached_values_map.clear();
    }
    // clear and remove cache
    ~Cache() {
        m_cached_values.clear();
        m_cached_values_map.clear();
    }

    // check whether the cache is valid. return true if valid
    bool Valid(void) const {
        return m_isValid;
    };
    // set cache invalid and clear it. or set it valid and do not clear it
    void SetInvalid(bool invalid = true) {
        m_isValid = !invalid;
        if (invalid) {
            m_cached_values.clear();
            m_cached_values_map.clear();
        }
    };

    // add value to cache. key - key of the cache record. value - cache record data
    bool AddValue(Key key, T value) {
        m_cached_values.push_back(value);
        m_cached_values_map[key] = value;
        return true;
    };

    /*
       find cached record value by key.
       if record is not found in cache -
           put false to *found and return an empty record object
       otherwise - put true to *found and return cached record
    */
    T FindValue(Key key, bool *found) {
        if (!m_isValid) {
            *found = false;
            return T();
        }
        typename std::map<Key, T>::iterator it = m_cached_values_map.find(key);
        if (it == m_cached_values_map.end()) {
            *found = false;
            return T();
        }
        *found = true;
        return m_cached_values_map[key];
    };

    // return array of cached records
    std::vector<T> const& CachedValues(void) {
        return m_cached_values;
    };
};

#endif