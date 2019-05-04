export PWD0=$PWD

cd $PWD0/crowker
./make.py && sudo ./make.py install

cd $PWD0/crow_publish
./make.py && sudo ./make.py install

cd $PWD0/crow_subscribe
./make.py && sudo ./make.py install

cd $PWD0/ctrans
./make.py && sudo ./make.py install