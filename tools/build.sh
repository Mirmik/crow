export PWD0=$PWD

cd $PWD0/crowker
./make.py

cd $PWD0/crow_publish
./make.py

cd $PWD0/crow_subscribe
./make.py

cd $PWD0/ctrans
./make.py