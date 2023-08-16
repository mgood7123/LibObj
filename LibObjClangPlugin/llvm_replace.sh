if [[ $# == 0 ]]
	then
		echo "please specify an LLVM number"
		exit
fi

if [[ ! -e "ClangParser/llvm/include_clang_$1" ]]
	then
		echo "LLVM import directory \"ClangParser/llvm/include_clang_$1\" does not exist, please run 'llvm_import.sh $1'"
		exit
fi

if [[ ! -e "ClangParser/llvm/lib_clang_____$1" ]]
	then
		echo "LLVM import directory \"ClangParser/llvm/lib_clang_____$1\" does not exist, please run 'llvm_import.sh $1'"
		exit
fi

if [[ ! -e "ClangParser/llvm/include_llvm__$1" ]]
	then
		echo "LLVM import directory \"ClangParser/llvm/include_llvm__$1\" does not exist, please run 'llvm_import.sh $1'"
		exit
fi

if [[ ! -e "ClangParser/llvm/lib_llvm______$1" ]]
	then
		echo "LLVM import directory \"ClangParser/llvm/lib_llvm______$1\" does not exist, please run 'llvm_import.sh $1'"
		exit
fi

replace() {
	LLVM_NUMBER="$5"

	INC_CLANG="include_clang_$LLVM_NUMBER"
	LIB_CLANG="lib_clang_____$LLVM_NUMBER"

	 INC_LLVM="include_llvm__$LLVM_NUMBER"
	 LIB_LLVM="lib_llvm______$LLVM_NUMBER"

	process_dir() {
		./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$1" -s "#include \"clang/" -r "#include \"$2/" -n --silent
		./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$1" -s "#include \"llvm/" -r "#include \"$3/" -n --silent
		./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$1" -s "namespace clang" -r "namespace clang_$4" -n --silent
		./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$1" -s "namespace llvm" -r "namespace llvm_$4" -n --silent
		./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$1" -s "clang::" -r "clang_$4::" -n --silent
		./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$1" -s "llvm::" -r "llvm_$4::" -n --silent
	}

	process_dir "$INC_CLANG" "$INC_CLANG" "$INC_LLVM" "$LLVM_NUMBER"
	process_dir "$INC_LLVM" "$INC_CLANG" "$INC_LLVM" "$LLVM_NUMBER"
	process_dir "$LIB_CLANG" "$INC_CLANG" "$INC_LLVM" "$LLVM_NUMBER"
	process_dir "$LIB_LLVM" "$INC_CLANG" "$INC_LLVM" "$LLVM_NUMBER"

	./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$INC_CLANG" -s "#include \"clang/" -r "#include \"$INC_CLANG/" -n --silent
	./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$INC_CLANG" -s "#include \"llvm/" -r "#include \"$INC_LLVM/" -n --silent
	./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$INC_CLANG" -s "namespace clang" -r "namespace clang_$LLVM_NUMBER" -n --silent
	./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$INC_CLANG" -s "namespace llvm" -r "namespace llvm_$LLVM_NUMBER" -n --silent
	./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$INC_LLVM" -s "#include \"clang/" -r "#include \"$INC_CLANG/" -n --silent
	./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/$INC_LLVM" -s "#include \"llvm/" -r "#include \"$INC_LLVM/" -n --silent
}

replace "clang_$1" "clang_____$1" "llvm__$1" "llvm______$1" "$1"

./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/include_clang_$1" -s "#include \"llvm/" -r "#include \"include_llvm___$1/" -n --silent
./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/lib_clang_____$1" -s "#include \"clang/" -r "#include \"include_clang_$1/" -n --silent
./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/lib_clang_____$1" -s "#include \"llvm/" -r "#include \"include_llvm___$1/" -n --silent
./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/include_llvm__$1" -s "#include \"clang/" -r "#include \"include_clang_$1/" -n --silent
./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/include_llvm__$1" -s "#include \"llvm/" -r "#include \"include_llvm___$1/" -n --silent
./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/lib_llvm______$1" -s "#include \"clang/" -r "#include \"include_clang_$1/" -n --silent
./ClangParser/FindReplace/release_EXECUTABLE/FindReplace -d "ClangParser/llvm/lib_llvm______$1" -s "#include \"llvm/" -r "#include \"include_llvm___$1/" -n --silent
