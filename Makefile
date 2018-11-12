cxx = g++
target = boeing_serv
cxxflags = -g -I . -I util -I connection -I message -I cache -I unittest \
	-I transport -I estimator -I pacing -I interface -I call -Wall -O2
cur_dir = .
util_dir = util
conn_dir = connection
msg_dir = message
trans_dir = transport
est_dir = estimator
pace_dir = pacing

src = $(wildcard $(util_dir)/*.cc)  \
	$(wildcard $(cur_dir)/*.cc)  \
	  $(wildcard $(conn_dir)/*.cc) \
	    $(wildcard $(msg_dir)/*.cc) \
	      $(wildcard $(trans_dir)/*.cc) \
	        $(wildcard $(est_dir)/*.cc) \
		  $(wildcard $(pace_dir)/*.cc) 


obj = $(patsubst %.cc, %.o, $(src))

$(target):$(obj)
	$(cxx) $(obj) -o $@ 


$(obj):%.o:%.cc
	$(cxx) -c $< -o $@ $(cxxflags) 

.PHONY:
	clean

clean:
	rm -rf $(util_dir)/*.o; \
	  rm -rf $(cur_dir)/*.o; \
	    rm -rf $(conn_dir)/*.o; \
	      rm -rf $(msg_dir)/*.o; \
	        rm -rf $(trans_dir)/*.o; \
	          rm -rf $(est_dir)/*.o; \
	            rm -rf $(pace_dir)/*.o; \
	              rm -rf $(target)
