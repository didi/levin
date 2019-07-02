Levin - A Quick Way to Bulk Loading
=======================


Description
-------

Levin provide a highly efficient solution in bulk loading scene.
A collection of STL-like containers implemented with high performance and memory efficiency.
Levin containers arrange memory layout SEQUENTIALLY, allocated at share memory region for reused inter-process.
which prolongs the lifetime of data resources, and prevents unnecessary time-consuming reload.

Loading with levin speed, let's go ahead.


Getting Started
-------

* Brief Intro for Levin Container

|          STD Container            |       Levin Container          |      Notes      |               Suggestion              |
| --------------------------------- | ------------------------------ | --------------- | ------------------------------------- |
| vector\<T\>                       | SharedVector\<T\>              | T is POD type   |                                       |
| set\<K, Compare\>                 | SharedSet\<K, Compare\>        | K is POD type   | use SharedHashSet if no comparison    |
| map\<K, V, Compare\>              | SharedMap\<K, V, Compare\>     | K/V is POD type | use SharedHashMap if no comparison    |
| unordered_set\<K, Hash, Pred\>    | SharedHashSet\<K, Hash, Pred\> | K is POD type   |                                       |
| unordered_map\<K, V, Hash, Pred\> | SharedHashMap\<K, V, Hash\>    | K/V is POD type |                                       |
| vector\<vector\<T\> \>            | SharedNestedVector\<T, SizeType\> | T is POD type; SizeType is unsigned integral type | need customized impl for combination; specified SizeType for memory space efficiency |
| unordered_map\<K, vector\<V\> \>  | SharedNestedHashMap\<K, V\>    | K/V is POD type | need customized impl for combination  |


* How to Dump Container

```c++
// vector dump demo
std::vector<int> vec_data = {1, 2, 3, 4, 5};
levin::SharedVector<int>::Dump("./vec_demo.dat", vec_data);
```

```c++
// map/hashmap dump demo
std::unordered_map<int64_t, int32_t> map_data = { {1, 100}, {2, 200}, {3, 300} };
// or std::map<int64_t, int32_t> map_data = { {1, 100}, {2, 200}, {3, 300} };
levin::SharedHashMap<int64_t, int32_t>::Dump("./map_demo.dat", map_data);
```


* How to Use Container

Tips: Levin container SHOULD be used as static data, NOT suggest to modify or reallocate.

```c++
// shared vector use demo
levin::SharedVector<int, levin::Md5Checker> vec("./vec_demo.dat");
if (vec.Init() == levin::SC_RET_OK && vec.Load() == levin::SC_RET_OK) {
    ...
}
vec.Destroy();
```

```c++
// shared hashmap use demo
auto map_ptr = new levin::SharedHashMap<int64_t, int32_t>("./map_demo.dat");
if (map_ptr->Init() == levin::SC_RET_OK && map_ptr->Load() == levin::SC_RET_OK) {
    ...
}
map_ptr->Destroy();
delete map_ptr;
```


* How to Manage a set of Containers

```c++
std::shared_ptr<levin::SharedVector<int> > ids_vec_ptr;
std::shared_ptr<levin::SharedContainerManager> share_memory_manager_ptr;

std::string path("./vec_demo.dat");
const auto &ret = share_memory_manager_ptr->Register(path, ids_vec_ptr);
if (levin::SC_RET_OK != ret) {
   return;
}

// do something
//ids_vec_ptr->size();
//int id = ids_vec_ptr[0];

share_memory_manager_ptr->Release();
```


Dependencies
-------

Levin depends on following packages, supported deps:

* gcc >= 4.8.5
* cmake >= 2.6.0
* boost >= 1.56.0
* openssl
* gtest

Compile
-------

* create directory build, cd build
* compile: cmake .. && make

Documentation
-------

About the details, please see [Wiki](../../wiki).

Contributing
-------

Welcome to contribute by creating issues or sending pull requests. See [Contributing Guide](CONTRIBUTING.md) for guidelines.

License
-------

Levin is licensed under the Apache License 2.0. See the [LICENSE](LICENSE) file.
