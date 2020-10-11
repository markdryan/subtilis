#!/bin/bash

echo "/*"
echo " * Copyright (c) 2020 Mark Ryan"
echo " *"
echo " * Licensed under the Apache License, Version 2.0 (the "License");"
echo " * you may not use this file except in compliance with the License."
echo " * You may obtain a copy of the License at"
echo " *"
echo " *     http://www.apache.org/licenses/LICENSE-2.0"
echo " *"
echo " * Unless required by applicable law or agreed to in writing, software"
echo " * distributed under the License is distributed on an "AS IS" BASIS,"
echo " * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied."
echo " * See the License for the specific language governing permissions and"
echo " * limitations under the License."
echo " */"

echo ""

echo "#include \"riscos_swi.h\""
echo ""
echo "/* clang-format off */"
echo ""
echo "const subtilis_arm_swi_t subtilis_riscos_swi_list[] = {"
while read line ; do
      echo -n -e "\t"
      echo -n "{ "
      echo -n $line
      echo " },"
done < swis.txt
echo "};"

echo ""
echo "const size_t subtilis_riscos_known_swis = sizeof(subtilis_riscos_swi_list) /"
echo -e "\tsizeof(subtilis_arm_swi_t);"
echo ""

echo "const size_t subtilis_riscos_swi_index[] = {"

i=0
while read line ; do
      echo -n -e "\t"
      echo -n $i
      echo -n ", "
      ((i=i+1))
      echo $line
done < swis.txt | sort -t, -k3 | cut -d, -f1,3 | sed -e 's/,/, \/\//g'

echo "};"
