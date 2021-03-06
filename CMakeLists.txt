## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

option(OSPRAY_MODULE_FIBER "Build 'Fibering' Module" ON)

if (OSPRAY_MODULE_FIBER)

  ospray_build_component(ospcommon)#Link ospcommon to bring in pthreads...

  find_package(Boost REQUIRED COMPONENTS fiber)

  include_directories(${Boost_INCLUDE_DIRS})

  ospray_create_application(ospFiberingConcurrentFor
    concurrent_for.cpp
  LINK
    ospray_common
    ${Boost_LIBRARIES}
  )

  ospray_create_application(ospFiberingRangeFor
    range_for.cpp
  LINK
    ospray_common
    ${Boost_LIBRARIES}
  )

  ospray_create_application(ospFiberingSimple
    simple.cpp
  LINK
    ospray_common
    ${Boost_LIBRARIES}
  )

  ospray_create_application(ospFiberingWorkSharing
    barrier.hpp
    work_sharing.cpp
  LINK
    ospray_common
    ${Boost_LIBRARIES}
  )

endif()
