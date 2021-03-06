d=""
c="./arm_simulator"

a="0"

for j in `ls arm*.c`
do
	d=`echo $d --debug $j`
done

for j in $@
do
	if [ $j = "-g" ]
	then
		c=`echo $c --gdb-port 50000 --irq-port 60000`
	elif [ $j = "-t" ]
	then 
		c=`echo $c --trace-registers --trace-memory --trace-state --trace-position`
	elif [ $j = "-d" ]
	then
		c=`echo $c $d`
	elif [ $j = "-h" ]
	then
		echo "simulator -g -t -d -h
-g : gdb-port 50000
-t : --trace-registers --trace-memory --trace-state --trace-position
-d : ajout tout les fichier arm*.c au debug
-h : cette aide"
		a="1"
	else
		c=`echo $c $j`
	fi
done

if [ $a = "0" ]
then 
	 $c
fi

#./arm_simulator --gdb-port 50000 --trace-registers --trace-memory --trace-state --trace-position $i
