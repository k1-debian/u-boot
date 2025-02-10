cat ../boards.cfg | grep "x1600_" | awk '{print $1}' > ci/x1600.cfg
cat ../boards.cfg | grep "x2600_" | awk '{print $1}' > ci/x2600.cfg

cat ../boards.cfg | grep "x2000_" | awk '{print $1}' > ci/x2000.cfg
cat ../boards.cfg | grep "x2100_" | awk '{print $1}' >> ci/x2000.cfg

cat ../boards.cfg | grep "m300" | awk '{print $1}' > ci/m300.cfg

cat ../boards.cfg | grep "x2500" | awk '{print $1}' > ci/x2500.cfg


for f in ci/*.cfg
do
    #echo $f
    filename=$(basename $f .cfg)
    #echo ci/$filename.yml
    python generate_ci.py $f ci/$filename.yml
done
