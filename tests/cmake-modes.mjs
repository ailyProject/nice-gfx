import fs from 'node:fs/promises';
import path from 'node:path';
import {spawnSync} from 'node:child_process';
import {root,run} from './host-tools.mjs';
const cmake=process.env.CMAKE||'cmake';
if(spawnSync(cmake,['--version'],{windowsHide:true}).status!==0)throw Error('CMake not found. Add it to PATH or set CMAKE to its executable.');
const work=path.join(root,'tests/.build/cmake');await fs.mkdir(work,{recursive:true});
await fs.writeFile(path.join(work,'CMakeLists.txt'),`cmake_minimum_required(VERSION 3.16)
# Test component registration directly without configuring an unrelated build
# generator. CONFIGURE_DEPENDS has no meaning during this script-mode fixture.
macro(file)
  set(file_args \${ARGV})
  list(REMOVE_ITEM file_args CONFIGURE_DEPENDS)
  _file(\${file_args})
endmacro()
macro(idf_component_register)
  cmake_parse_arguments(REG "" "" "SRCS;INCLUDE_DIRS;REQUIRES" \${ARGN})
  set(COMPONENT_LIB fixture)
endmacro()
function(target_compile_features)
endfunction()
function(target_compile_options)
endfunction()
include("${path.join(root,'CMakeLists.txt').replaceAll('\\','/')}")
if(NICE_GFX_ARDUINO_COMPONENT)
  list(FIND REG_REQUIRES "\${NICE_GFX_ARDUINO_COMPONENT}" found)
  if(found LESS 0)
    message(FATAL_ERROR "Arduino component not registered")
  endif()
endif()
if(CMAKE_BUILD_EARLY_EXPANSION)
  if(REG_SRCS)
    message(FATAL_ERROR "Sources expanded during early requirements pass")
  endif()
else()
  if(NOT REG_SRCS)
    message(FATAL_ERROR "No component sources registered")
  endif()
  foreach(source IN LISTS REG_SRCS)
    if(NOT NICE_GFX_ARDUINO_COMPONENT AND source MATCHES "/Platform/(Arduino|STM32|RP2040)/")
      message(FATAL_ERROR "Native build includes Arduino-only sources")
    endif()
  endforeach()
  set(fonts \${REG_SRCS})
  list(FILTER fonts INCLUDE REGEX "/Fonts/Mono/GFX_Font_")
  list(LENGTH fonts font_count)
  if(NOT NICE_GFX_ENABLE_FONTS OR NICE_GFX_FONT_SOURCES STREQUAL "NONE")
    set(expected_fonts 0)
  elseif(NICE_GFX_FONT_SOURCES)
    set(selected_fonts \${NICE_GFX_FONT_SOURCES})
    list(REMOVE_DUPLICATES selected_fonts)
    list(LENGTH selected_fonts expected_fonts)
  else()
    file(GLOB bundled_fonts "${path.join(root,'src/Fonts/Mono/GFX_Font_*.cpp').replaceAll('\\','/')}")
    list(LENGTH bundled_fonts expected_fonts)
  endif()
  if(NOT font_count EQUAL expected_fonts)
    message(FATAL_ERROR "Font source selection failed: \${font_count} instead of \${expected_fonts}")
  endif()
  set(ui_sources \${REG_SRCS})
  list(FILTER ui_sources INCLUDE REGEX "/UI/")
  if(NICE_GFX_ENABLE_UI)
    foreach(unit View Image)
      list(FIND ui_sources "${path.join(root,'src/UI').replaceAll('\\','/')}/\${unit}.cpp" found)
      if(found LESS 0)
        message(FATAL_ERROR "UI enabled without \${unit}.cpp")
      endif()
    endforeach()
  elseif(ui_sources)
    message(FATAL_ERROR "UI disabled but UI translation units are registered")
  endif()
endif()
list(FIND REG_REQUIRES esp_driver_ppa ppa)
if(IDF_TARGET STREQUAL "esp32s3" AND NOT ppa LESS 0)
  message(FATAL_ERROR "S3 build depends on PPA")
endif()
if(IDF_TARGET STREQUAL "esp32s31" AND ppa LESS 0)
  message(FATAL_ERROR "S31 build omits PPA")
endif()
`);
const modes=[[],['-DNICE_GFX_ARDUINO_COMPONENT=arduino'],['-DNICE_GFX_ARDUINO_COMPONENT=espressif__arduino-esp32'],
  ['-DCMAKE_BUILD_EARLY_EXPANSION=TRUE'],['-DNICE_GFX_ARDUINO_COMPONENT=arduino','-DCMAKE_BUILD_EARLY_EXPANSION=TRUE'],
  ['-DNICE_GFX_FONT_SOURCES=Fixed6x13;GfxSansCN16;Fixed6x13'],['-DIDF_TARGET=esp32s31'],
  ['-DNICE_GFX_BUILD_PROFILE=full'],['-DNICE_GFX_BUILD_PROFILE=graphics'],['-DNICE_GFX_BUILD_PROFILE=minimal'],
  ['-DNICE_GFX_BUILD_PROFILE=minimal','-DNICE_GFX_ENABLE_UI=ON'],
  ['-DNICE_GFX_BUILD_PROFILE=minimal','-DNICE_GFX_ENABLE_FONTS=ON','-DNICE_GFX_FONT_SOURCES=Fixed6x13'],
  ['-DNICE_GFX_ENABLE_UI=OFF'],['-DNICE_GFX_FONT_SOURCES=NONE'],
  ['-DNICE_GFX_BUILD_PROFILE=graphics','-DNICE_GFX_ENABLE_UI=ON'],
  ['-DNICE_GFX_BUILD_PROFILE=minimal','-DCMAKE_BUILD_EARLY_EXPANSION=TRUE']];
for(const args of modes)run(cmake,['-DIDF_TARGET=esp32s3',...args,'-P',path.join(work,'CMakeLists.txt')],{stdio:'pipe'});
for(const [args,message] of [
  [['-DNICE_GFX_FONT_SOURCES=MissingFont'],'Unknown bundled font'],
  [['-DNICE_GFX_FONT_SOURCES=../Invalid'],'Invalid NICE_GFX_FONT_SOURCES entry'],
  [['-DNICE_GFX_BUILD_PROFILE=invalid'],'Unknown NICE_GFX_BUILD_PROFILE'],
  [['-DNICE_GFX_BUILD_PROFILE=minimal','-DNICE_GFX_FONT_SOURCES=Fixed6x13'],'requires NICE_GFX_ENABLE_FONTS=ON'],
]){
  const invalid=spawnSync(cmake,[...args,'-P',path.join(work,'CMakeLists.txt')],{windowsHide:true,encoding:'utf8'});
  if(invalid.error)throw invalid.error;
  if(invalid.status===0||!invalid.stdout.concat(invalid.stderr).includes(message))throw Error(`Invalid configuration was not rejected: ${args.join(' ')}`);
}
console.log(`PASS ${modes.length} CMake modes: profiles, overrides, native/Arduino, early expansion, fonts and S31 PPA; 4 invalid configurations rejected.`);
