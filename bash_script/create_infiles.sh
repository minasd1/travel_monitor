#! /bin/bash  
args=("$@")

if [ "$#" -ne 3 ]               #number of arguments must be equal to 3
then
    echo "You must give 3 arguments!"
    exit 1
fi

inputFile=${args[0]}

#if given fileName does not exist
if ! [[ -f "$inputFile" ]]
then
                              
    echo "$inputFile doesn't exist"
    exit 1    
    
fi

numFilesPerDirectory=${args[2]}
if [[ "$numFilesPerDirectory" =~ ^[+-]?[0-9]+([.][0-9]+)?$ ]] #check if given argument is an integer
then
    if [ "$numFilesPerDirectory" -le 0 ]                      #check if it is positive
    then
        echo "3rd argument must greater than 0!"
        exit 1
    fi
else
    echo "3rd argument must be an integer!"
    exit 1
fi

#if given directory already exists
input_dir=${args[1]}
if [[ -d "$input_dir" ]]
then
    echo "Input directory already exists!"
    exit 1
else                                                            #make a directory
    mkdir $input_dir
fi



templist=$(cut -d " " -f 4 $inputFile)                                          
countrieslist=($(echo "${templist[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' '))


for i in "${countrieslist[@]}"                                  
do
    mkdir $input_dir/$i                                         #make a subdirectory for every country
    for ((num = 1 ; num <= $numFilesPerDirectory ; num++))  
    do
        touch $input_dir/$i/"$i-$num".txt                       #make numFilesPerDirectory files for every country's subdirectory
    done            
done


num_countries=${#countrieslist[@]}

mapfile -t lines_of_file < $inputFile

cd $input_dir

for c in "${countrieslist[@]}"
do
    count=1                                                     
    for line in "${lines_of_file[@]}"
    do  
        if [ "$count" -gt "$numFilesPerDirectory" ]             #we have reached the max-number file
        then
            count=1                                             #write back to the first one
        fi

        country=$(echo $line | cut -d " " -f 4)                 #if country from current record of file matches current country from list
        if [[ "$country" == "$c"  ]]
        then
            cd $country                                         #then open the corresponding folder
            echo $line >> "$country-$count".txt                 #and write the record to current file
            count=$((count+1))                                  #increment counter(round-robin implementation)
            cd ..
        fi
        
    done
done

cd ..







