if [[ $# != "2" ]]
    then
        echo "please specify an LLVM source directory and an LLVM id"
        exit
fi

if [[ ! -e "$1/clang" ]]
	then
		echo "LLVM source \"$1/clang\" does not exist"
		exit
fi

if [[ ! -e "$1/clang/include" ]]
	then
		echo "LLVM source \"$1/clang/include\" does not exist"
		exit
fi

if [[ ! -e "$1/clang/lib" ]]
	then
		echo "LLVM source \"$1/clang/lib\" does not exist"
		exit
fi

if [[ ! -e "$1/llvm" ]]
    then
        echo "LLVM source \"$1/llvm\" does not exist"
        exit
fi

if [[ ! -e "$1/llvm/include" ]]
	then
		echo "LLVM source \"$1/llvm/include\" does not exist"
		exit
fi

if [[ ! -e "$1/llvm/lib" ]]
	then
		echo "LLVM source \"$1/llvm/lib\" does not exist"
		exit
fi

if [[ ! -e "ClangParser/llvm" ]]
    then
        mkdir ClangParser/llvm
fi

copy() {
    if [[ ! -e "$1/$3" ]]
        then
            echo "LLVM source \"$1/$3\" does not exist"
            exit
    fi

    if [[ ! -e "$1/$3/$4" ]]
        then
            echo "LLVM source \"$1/$3/$4\" does not exist"
            exit
    fi
    if [[ -e "ClangParser/llvm/$2" ]]
        then
            echo "removing ClangParser/llvm/$2 ..."
            rm -rf "ClangParser/llvm/$2"
    fi

    echo "copying into ClangParser/llvm/$2 ..."
    cp -r "$1/$3/$4" "ClangParser/llvm/$2"
}

copy "$1" "include_clang_$2" clang include
copy "$1" "lib_clang_____$2" clang lib
copy "$1" "include_llvm__$2" llvm include
copy "$1" "lib_llvm______$2" llvm lib

./llvm_replace.sh "$2"