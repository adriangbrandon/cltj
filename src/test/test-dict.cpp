//
// Created by adrian on 26/11/24.
//

#include <dict/dict_map2.hpp>
#include <iostream>
#include <string>

int main() {

    //Interactive insert, extract and delete

    std::string comando, clave;
    uint64_t id;

    dict::tiny_map2 map_SO;
    std::cout << "Comandos: insert <clave>, get <clave>, extract <id>, eliminate <clave>, exit\n";
    while (true) {
        std::cout << "> ";
        std::cin >> comando;
        if (comando == "insert") {
            std::cin >> clave;
            id = map_SO.insert(clave);
            std::cout << "ID: " << id << std::endl;
        } else if (comando == "get") {
            std::cin >> clave;
            id = map_SO.get_or_insert(clave);
            std::cout << "ID: " << id << std::endl;
        } else if (comando == "extract") {
            std::cin >> id;
            std::cout << "Clave: " << map_SO.extract(id) << std::endl;
        } else if (comando == "eliminate") {
            std::cin >> clave;
            map_SO.eliminate(clave);
            std::cout << "Eliminado\n";
        } else if (comando == "exit") {
            break;
        } else {
            std::cout << "Comando desconocido\n";
        }
    }

    /* dict::tiny_map map_SO;
     auto id_adrian = map_SO.insert("adrian");
    std::cout << "id_adrian: " << id_adrian << std::endl;
    auto id_pedro = map_SO.insert("pedro");
    std::cout << "id_pedro: " << id_pedro << std::endl;

    id_adrian = map_SO.get_or_insert("adrian");
    auto id_fari = map_SO.get_or_insert("fari");
    std::cout << "id_adrian: " << id_adrian << std::endl;
    std::cout << "id_fari: " << id_fari << std::endl;
    std::cout << map_SO.extract(1) << std::endl;
    std::cout << map_SO.extract(1) << std::endl;
    map_SO.eliminate("adrian");

    auto id_diego = map_SO.insert("diego");
    std::cout << "id_diego: " << id_diego << std::endl;

    std::cout << map_SO.extract(1) << std::endl;
    std::cout << map_SO.extract(2) << std::endl;
    std::cout << map_SO.extract(3) << std::endl;*/

}