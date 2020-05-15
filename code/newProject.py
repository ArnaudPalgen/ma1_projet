import sys
import os

if __name__ == "__main__":
    project_name = sys.argv[1]
    main_file_name = sys.argv[2]

    os.mkdir(project_name)
    os.chdir(project_name)
    os.mkdir("main")
    os.mkdir("logs")

    f = open("CMakeLists.txt", "w")
    f.write("cmake_minimum_required(VERSION 3.5)\n\ninclude($ENV{IDF_PATH}/tools/cmake/project.cmake)\nproject("+project_name+")\n")
    f.close()

    f = open("Makefile", "w")
    f.write("PROJECT_NAME := "+project_name+"\n\ninclude $(IDF_PATH)/make/project.mk\n")
    f.close()

    os.chdir("main")

    f = open(main_file_name+".c", "w")
    f.write("""#include <string.h>\n#include "esp_wifi.h"\n#include "esp_system.h"\n#include "esp_event_loop.h"\n#include "esp_log.h"\n#include "esp_mesh.h"\n#include "esp_mesh_internal.h"\n#include "nvs_flash.h"\n\nvoid app_main(void){\n    printf("HELLO HOME !");\n}""")
    f.close()

    f = open("CMakeLists.txt","w")
    f.write("set(COMPONENT_SRCS \""+main_file_name+".c\")\nset(COMPONENT_ADD_INCLUDEDIRS \"\")\n\nregister_component()\n")
    f.close()