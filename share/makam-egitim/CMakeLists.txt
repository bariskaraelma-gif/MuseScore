# SPDX-License-Identifier: GPL-3.0-only
#
# TurkScore (Makam Nota): the lessons of the training tab on the Learn page
# (LearnPageModel::makamLessons). Each folder is a section, each .mscx / .mscz in it a lesson,
# with an optional .txt of the same name for the card texts. The lessons are made from the
# sources in makam/egitim (python makam/egitim/dersleri_uret.py).

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
      DESTINATION ${Mscore_SHARE_NAME}${Mscore_INSTALL_NAME}makam-egitim
      FILES_MATCHING
      PATTERN "*.mscx"
      PATTERN "*.mscz"
      PATTERN "*.txt"
      PATTERN "CMakeLists.txt" EXCLUDE
      )
