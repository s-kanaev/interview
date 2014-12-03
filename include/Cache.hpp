#ifndef _CACHE_HPP_
#define _CACHE_HPP_

template<typename Key, typename T>
class Cache {
protected:
    bool m_isValid;
    std::vector<T> m_cached_values;
    std::map<Key, T> m_cached_values_map;

public:
    Cache();
    ~Cache();

    // check whether the cache is valid
    bool Valid(void) const {
        return m_isValid;
    };
    // set cache invalid and clear it
    void SetInvalid(bool invalid = true) {
        m_isValid = !invalid;
        m_cached_values.clear();
        m_cached_values_map.clear();
    };

    // add value to cache
    bool AddValue(Key key, T value) {
        m_cached_values.push_back(value);
        m_cached_values_map[key] = value;
        return true;
    };

    // find cached value by key
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

    // return array of cached values
    std::vector<T> const& CachedValues(void) {
        return m_cached_values;
    };
};

#endif