/*
  javascript display of raw SMA webbox data
  Copyright Andrew Tridgell 2010
  Released under GNU GPL v3 or later
 */


var is_chrome = (navigator.userAgent.toLowerCase().indexOf('chrome') != -1);

/*
  return the variables set after a '#' as a hash
 */
function parse_hashvariables() {
   var ret = [];
   var url = window.location.hash.slice(1);
   var vars = url.split(';');
   for (var i=0; i<vars.length; i++) {
     var x = vars[i].split('=');
     if (x.length == 2) {
       ret[x[0]] = x[1];
     }
   }
   return ret;
}

hashvars = parse_hashvariables();

/*
  rewrite the URL hash so you can bookmark particular dates
 */
function rewrite_hashvars(vars) {
  var hash = '';
  for (var x in vars) {
    hash += '' + x + '=' + vars[x] + ';';
  }
  hash = hash.slice(0,hash.length-1);
  window.location.hash = hash;
}

/*
  round a date back to midnight
 */
function date_round(d) {
  var d2 = new Date(d);
  d2.setHours(0);
  d2.setMinutes(0);
  d2.setSeconds(0);
  d2.setMilliseconds(0);
  return d2;
}

/*
  the date in Canberra
 */
function canberraDate() {
  var d = new Date();
  return date_round(new Date(d.getTime() + (tz_difference*60*60*1000)));
}

/*
  work out timezone
 */
pvdate = date_round(new Date());
period_days = 1;
auto_averaging = 1;
tz_difference = 11 + (pvdate.getTimezoneOffset()/60);



/* marker for whether we are in a redraw with new data */
in_redraw = false;

/*
  show a HTML heading
 */
function heading(h) {
  if (!in_redraw) {
    document.write("<h3><a STYLE='text-decoration:none' href=\"javascript:toggle_div('"+h+"')\"><img src='icons/icon_unhide_16.png' width='16' height='16' border='0' id='img-"+h+"'></a>&nbsp;"+h+"</h3>");
  }
}

/*
  create a div for a graph
 */
function graph_div(divname) {
  if (!in_redraw) {
    document.write(
		   '<table><tr>' +
		   '<td valign="top"><div id="' + divname + '" style="width:700px; height:350px;"></div></td>' +
		   '<td valign="top">&nbsp;&nbsp;</td>' +
		   '<td valign="top"><div id="' + divname + ':labels"></div></td>' +
		   '</tr></table>\n');
  }
}


/*
  hide/show a div
 */
function hide_div(divname, hidden) {
  var div = document.getElementById(divname);
  if (hidden) {
    div.style.display = "none";
  } else {
    div.style.display = "block";
  }
}

/* unhide the loading div when busy */
loading_counter = 0;

function loading(busy) {
  if (busy) {
    loading_counter++;
    if (loading_counter == 1) {
      started_loading=new Date();
      hide_div("loading", false);
    }
  } else {
    if (loading_counter > 0) {
      loading_counter--;
      if (loading_counter == 0) {
	hide_div("loading", true);
	var d = new Date();
	var load_time = d.getTime() - started_loading.getTime();
	writeDebug("Loading took: " + (load_time/1000));
      }
    }
  }
}


/* a global call queue */
global_queue = new Array();
graph_queue = new Array();

/*
  run the call queue
 */
function run_queue(q) {
  var qe = q[0];
  var t_start = new Date();
  qe.callback(qe.arg);
  var t_end = new Date();
  q.shift();
  if (q.length > 0) {
    var tdelay = (t_end.getTime() - t_start.getTime())/4;
    if (tdelay < 1) {
      run_queue(q);
    } else {
      setTimeout(function() { run_queue(q);}, tdelay);    
    }
  }
}

/*
  queue a call. This is used to serialise long async operations in the
  browser, so that you get less timeouts. It is especially needed on
  IE, where the canvas widget is terribly slow.
 */
function queue_call(q, callback, arg) {
  q.push( { callback: callback, arg : arg });
  if (q.length == 1) {
    setTimeout(function() { run_queue(q); }, 1);
  }
}

function queue_global(callback, arg) {
  queue_call(global_queue, callback, arg);
}

function queue_graph(callback, arg) {
  queue_call(graph_queue, callback, arg);
}


/*
  date parser. Not completely general, but good enough
 */
function parse_date(s, basedate) {
  if (s.length == 5 && s[2] == ':') {
    /* its a time since midnight */
    var h = (+s.substring(0, 2));
    var m = (+s.substring(3));
    var d = basedate.getTime() + 1000*(h*60*60 + m*60);
    return d;
  }
  if (s.length == 8 && s[2] == ':' && s[5] == ':') {
    /* its a time since midnight */
    var h = (+s.substring(0, 2));
    var m = (+s.substring(3, 5));
    var sec = (+s.substring(6));
    var d = basedate.getTime() + 1000*(h*60*60 + m*60 + sec);
    return d;
  }
  if (s.search("-") != -1) {
    s = s.replace("-", "/", "g");
  }
  if (s[2] == '/') {
    var x = s.split('/');
    var d = new Date();
    d.setDate(+x[0]);
    d.setMonth(x[1]-1);
    d.setYear(+x[2]);
    return date_round(d);
  }
  if (s.search("/") != -1) {
    return date_round(new Date(s));
  }
  /* assume time in milliseconds since 1970 */
  return (+s);
};


/*
  return a YYYY-MM-DD date string
 */
function date_YMD(d) {
  return '' + intLength(d.getFullYear(),4) + '-' + intLength(d.getMonth()+1,2) + '-' + intLength(d.getDate(),2);
}

/*
  parse the date portion of a filename which starts with YYYY-MM-DD after a directory
 */
function filename_date(filename) {
  var idx = filename.lastIndexOf("/");
  if (idx != -1) {
    filename = filename.substring(idx+1);
  }
  if (filename[4] == '-' && filename[7] == '-') {
    /* looks like a date */
    var d = date_round(new Date());
    d.setYear(+filename.substring(0,4));
    d.setMonth(filename.substring(5,7)-1);
    d.setDate(filename.substring(8,10));
    return d;
  }
  return pvdate;
}


/*
  parse a CSV value
 */
function parse_value(s) {
  if (s.substring(0,1) == '"') {
    s = unescape(s.substring(1,s.length-1));
    return s;
  }
  if (s == '') {
    return null;
  }
  var n = new Number(s);
  if (isNaN(n)) {
    return s;
  }
  return n;
}

/* keep a cache of loaded CSV files */
CSV_Cache = new Array();


/*
  load a CSV file, returning column names and data via a callback
 */
function load_CSV(filename, callback) {

  /* maybe its in the global cache? */
  if (CSV_Cache[filename] !== undefined) {

    if (CSV_Cache[filename].pending) {
      /* its pending load by someone else. Add ourselves to the notify
	 queue so we are told when it is done */
      CSV_Cache[filename].queue.push({filename:filename, callback:callback});
      return;
    }

    /* its ready in the cache - return it via a delayed callback */
    if (CSV_Cache[filename].data == null) {
      var d = { filename: CSV_Cache[filename].filename,
		labels:   null,
		data:     null };
      queue_global(callback, d);
    } else {
      var d = { filename: CSV_Cache[filename].filename,
		labels:   CSV_Cache[filename].labels.slice(0),
		data:     CSV_Cache[filename].data.slice(0) };
      queue_global(callback, d);
    }
    return;
  }

  /* mark this one pending */
  CSV_Cache[filename] = { filename:filename, pending: true, queue: new Array()};

  /*
    async callback when the CSV is loaded
   */
  function load_CSV_callback(caller) {
    var data = new Array();
    var labels = new Array();

    if (filename.search(".csv") != -1) {
      var csv = caller.r.responseText.split(/\n/g);

      /* assume first line is column labels */
      labels = csv[0].split(/,/g);
      for (var i=0; i<labels.length; i++) {
	labels[i] = labels[i].replace(" ", "&nbsp;", "g");
      }

      /* the rest is data, we assume comma separation */
      for (var i=1; i<csv.length; i++) {
	var row = csv[i].split(/,/g);
	if (row.length <= 1) {
	  continue;
	}
	data[i-1] = new Array();
	data[i-1][0] = parse_date(row[0], caller.basedate);
	for (var j=1; j<row.length; j++) {
	  data[i-1][j] = parse_value(row[j]);
	}
      }
    } else {
      var xml = caller.r.responseText.split(/\n/g);
      var last_date = 0;
      var addday = false;
      for (var i=0; i < xml.length; i++) {
	if (xml[i].search("<hist>") != -1) {
	  continue;
	}
	var row = xml[i].split("<");
	var num_labels = 0;
	var prefix = "";
	if (row.length < 2) {
	  continue;
	}
	var rdata = new Array();
	for (var j=1; j<row.length; j++) {
	  var v = row[j].split(">");
	  if (v[0].substring(0,1) == "/") {
	    var tag = v[0].substring(1);
	    if (prefix.substring(prefix.length-tag.length) == tag) {
	      prefix = prefix.substring(0, prefix.length-tag.length);
	      if (prefix.substring(prefix.length-1) == ".") {
		prefix = prefix.substring(0, prefix.length-1);		
	      }
	    }
	    continue;
	  } else if (v[1] == "") {
	    if (prefix != "") {
	      prefix += ".";
	    }
	    prefix += v[0];
	    continue;
	  }
	  if (v[0] == "stime") {
	    var dtime = parse_date(v[1], caller.basedate);
	    labels[0] = "time";
	    rdata[0] = dtime;
	  } else if (v[0] == "time") {
	    var dtime = parse_date(v[1], caller.basedate);
	    if (last_date != 0 && dtime < last_date) {
	      if (i < xml.length/2) {
		/*
		  the earlier points were from the last night
		 */
		for (var ii=0; ii<data.length; ii++) {
		  data[ii][0] = data[ii][0] - 24*3600*1000;
		}
		writeDebug("subtraced day from: " + data.length);
	      } else {
		/* we've gone past the end of the day */
		addday = true;
	      }
	    }
	    if (addday) {
	      dtime += 24*3600*1000;
	    }
	    last_date = dtime;
	    labels[0] = "time";
	    rdata[0] = dtime;
	  } else if (v[1] != "") {
	    labels[num_labels+1] = prefix + "." + v[0];
	    rdata[num_labels+1] = parseFloat(v[1]);
	    num_labels++;
	  }
	}
	data[data.length] = rdata;
      }
    }
    
    /* save into the global cache */
    CSV_Cache[caller.filename].labels = labels;
    CSV_Cache[caller.filename].data   = data;

    /* give the caller a copy of the data (via slice()), as they may
       want to modify it */
    var d = { filename: CSV_Cache[filename].filename,
	      labels:   CSV_Cache[filename].labels.slice(0),
	      data:     CSV_Cache[filename].data.slice(0) };
    queue_global(caller.callback, d);

    /* fire off any pending callbacks */
    while (CSV_Cache[caller.filename].queue.length > 0) {
      var qe = CSV_Cache[caller.filename].queue.shift();
      var d = { filename: filename,
		labels:   CSV_Cache[filename].labels.slice(0),
		data:     CSV_Cache[filename].data.slice(0) };
      queue_global(qe.callback, d);
    }
    CSV_Cache[caller.filename].pending = false;
    CSV_Cache[caller.filename].queue   = null;
  }

  /* make the async request for the file */
  var caller = new Object();
  caller.r = new XMLHttpRequest();
  caller.callback = callback;
  caller.filename = filename;
  caller.basedate = filename_date(filename);

  /* check the status when that returns */
  caller.r.onreadystatechange = function() {
    if (caller.r.readyState == 4) {
      if (caller.r.status == 200) {
	queue_global(load_CSV_callback, caller);
      } else {
	/* the load failed */
	queue_global(caller.callback, { filename: filename, data: null, labels: null });
	while (CSV_Cache[caller.filename].queue.length > 0) {
	  var qe = CSV_Cache[caller.filename].queue.shift();
	  var d = { filename: CSV_Cache[filename].filename,
		    labels:   null,
		    data:     null };
	  queue_global(qe.callback, d);
	}
	CSV_Cache[caller.filename].pending = false;
	CSV_Cache[caller.filename].queue   = null;
	CSV_Cache[caller.filename].data   = null;
	CSV_Cache[caller.filename].labels   = null;
      }
    }
  }
  caller.r.open("GET",filename,true);
  caller.r.send(null);
}

function array_equal(a1, a2) {
  if (a1.length != a2.length) {
    return false;
  }
  for (var i=0; i<a1.length; i++) {
    if (a1[i] != a2[i]) {
      return false;
    }
  }
  return true;
}

/*
  combine two arrays that may have different labels
 */
function combine_arrays(a1, l1, a2, l2) {
  if (array_equal(l1, l2)) {
    return a1.concat(a2);
  }
  /* we have two combine two arrays with different labels */
  var map = new Array();
  for (var i=0; i<l1.length; i++) {
    map[i] = l2.indexOf(l1[i]);
  }
  ret = a1.slice(0);
  for (var y=0; y<a2.length; y++) {
    var r = new Array();
    for (var x=0; x<l1.length; x++) {
      if (map[x] == -1) {
	r[x] = null;
      } else {
	r[x] = a2[y][map[x]];
      }
    }
    ret.push(r);
  }
  return ret;
}

/*
  load a comma separated list of CSV files, combining the data
 */
function load_CSV_array(filenames, callback) {
  var c = new Object();
  c.filename = filenames;
  c.files = filenames.split(',');
  c.callback = callback;
  c.data = new Array();
  c.labels = new Array();
  c.count = 0;

  /*
    async callback when a CSV is loaded
   */
  function load_CSV_array_callback(d) {
    c.count++;
    var i = c.files.indexOf(d.filename);
    c.data[i] = d.data;
    c.labels[i] = d.labels;
    if (c.count == c.files.length) {
      var ret = { filename: c.filename, data: c.data[0], labels: c.labels[0]};
      for (var i=1; i<c.files.length; i++) {
	if (c.data[i] != null) {
	  if (ret.data == null) {
	    ret.data = c.data[i];
	    ret.labels = c.labels[i];
	  } else {
	    ret.data = combine_arrays(ret.data, ret.labels, c.data[i], c.labels[i]);
	  }
	}
      }
      if (ret.data == null) {
	hide_div("nodata", false);
      } else {
	hide_div("nodata", true);
      }
      queue_global(c.callback, ret);
    }
  }

  for (var i=0; i<c.files.length; i++) {
    load_CSV(c.files[i], load_CSV_array_callback);
  }
}

/*
  format an integer with N digits by adding leading zeros
  javascript is so lame ...
 */
function intLength(v, N) {
  var r = v + '';
  while (r.length < N) {
    r = "0" + r;
  }
  return r;
}


/*
  return the position of v in an array or -1
 */
function pos_in_array(a, v) {
  for (var i=0; i<a.length; i++) {
    if (a[i] == v) {
      return i;
    }
  }
  return -1;
}

/*
  see if v exists in array a
 */
function in_array(a, v) {
  return pos_in_array(a, v) != -1;
}


/*
  return a set of columns from a CSV file
 */
function get_csv_data(filenames, columns, callback) {
  var caller = new Object();
  caller.d = new Array();
  caller.columns = columns.slice(0);
  caller.filenames = filenames.slice(0);
  caller.callback = callback;

  /* initially blank data - we can tell a load has completed when it
     is filled in */
  for (var i=0; i<caller.filenames.length; i++) {
    caller.d[i] = { filename: caller.filenames[i], labels: null, data: null};
  }

  /* process one loaded CSV, mapping the data for
     the requested columns */
  function process_one_csv(d) {
    var labels = new Array();

    if (d.data == null) {
      queue_global(caller.callback, d);
      return;
    }

    /* form the labels */
    labels[0] = "Time";
    for (var i=0; i<caller.columns.length; i++) {
      labels[i+1] = caller.columns[i];
    }

    /* get the column numbers */
    var cnums = new Array();
    cnums[0] = 0;
    for (var i=0; i<caller.columns.length; i++) {
      cnums[i+1] = pos_in_array(d.labels, caller.columns[i]);
    }
  
    /* map the data */
    var data = new Array();
    for (var i=0; i<d.data.length; i++) {
      data[i] = new Array();
      for (var j=0; j<cnums.length; j++) {
	data[i][j] = d.data[i][cnums[j]];
      }
    }
    d.data = data;
    d.labels = labels;

    for (var f=0; f<caller.filenames.length; f++) { 
      if (d.filename == caller.d[f].filename) {
	caller.d[f].labels = labels;
	caller.d[f].data = data;
      }
    }

    /* see if all the files are now loaded */
    for (var f=0; f<caller.filenames.length; f++) { 
      if (caller.d[f].data == null) {
	return;
      }
    }

    /* they are all loaded - make the callback */
    queue_global(caller.callback, caller.d);
  }

  /* start the loading */
  for (var i=0; i<caller.filenames.length; i++) {
    load_CSV_array(caller.filenames[i], process_one_csv);
  }
}


/*
  apply a function to a set of data, giving it a new label
 */
function apply_function(d, func, label) {
  if (func == null) {
    return;
  }
  for (var i=0; i<d.data.length; i++) {
    var r = d.data[i];
    d.data[i] = r.slice(0,1);
    d.data[i][1] = func(r.slice(1))
  }
  d.labels = d.labels.slice(0,1);
  d.labels[1] = label;
}


/* currently displayed graphs, indexed by divname */
global_graphs = new Array();

/*
  find a graph by divname
 */
function graph_find(divname) {
  for (var i=0; i<global_graphs.length; i++) {
    var g = global_graphs[i];
    if (g.divname == divname) {
      return g;
    }
  }
  return null;
}

function nameAnnotation(ann) {
  return "(" + ann.series + ", " + ann.xval + ")";
}

annotations = [];

/*
  try to save an annotation via annotation.cgi
 */
function save_annotation(ann) {
  var r = new XMLHttpRequest();
  r.open("GET", 
	 "cgi/annotation.cgi?series="+escape(ann.series)+"&xval="+ann.xval+"&text="+escape(ann.text), true);
  r.send(null);  
}

function round_annotations() {
  for (var i=0; i<annotations.length; i++) {
    annotations[i].xval = round_time(annotations[i].xval, defaultAttrs.averaging);
  }
}

/*
  load annotations from annotations.csv
 */
function load_annotations(g) {
  function callback(d) {
    var anns_by_name = new Array();
    annotations = [];
    for (var i=0; i<d.data.length; i++) {
      var xval = d.data[i][0] + (tz_difference*60*60*1000);
      xval = round_time(xval, defaultAttrs.averaging);
      if (xval.valueOf() < pvdate.valueOf() || 
	  xval.valueOf() >= (pvdate.valueOf() + (24*60*60*1000))) {
	continue;
      }
      var ann = {
      xval: xval.valueOf(),
      series: d.data[i][1],
      shortText: '!',
      text: decodeURIComponent(d.data[i][2])
      };
      var a = anns_by_name[nameAnnotation(ann)];
      if (a == undefined) {
	anns_by_name[nameAnnotation(ann)] = annotations.length;
	annotations.push(ann);
      } else {
	annotations[a] = ann;
	if (ann.text == '') {
	  annotations.splice(a,1);
	}
      }
    }
    for (var i=0; i<global_graphs.length; i++) {
      var g = global_graphs[i];
      g.setAnnotations(annotations);
    }
  }

  load_CSV("../CSV/annotations.csv", callback);
}

function annotation_highlight(ann, point, dg, event) {
  saveBg = ann.div.style.backgroundColor;
  ann.div.style.backgroundColor = '#ddd';
}

function annotation_unhighlight(ann, point, dg, event) {
  ann.div.style.backgroundColor = saveBg;
}

/*
  handle annotation updates
 */
function annotation_click(ann, point, dg, event) {
  ann.text = prompt("Enter annotation", ann.text);
  if (ann.text == null) {
    return;
  }
  for (var i=0; i<annotations.length; i++) {
    if (annotations[i].xval == ann.xval && annotations[i].series == ann.series) {
      annotations[i].text = ann.text;
      if (ann.text == '' || ann.text == null) {
	ann.text = '';
	writeDebug("removing annnotation");
	annotations.splice(i,1);
	i--;
      }
    }
  }
  for (var i=0; i<global_graphs.length; i++) {
    var g = global_graphs[i];
    if (g.series_names.indexOf(ann.series) != -1) {
      g.setAnnotations(annotations);
    }
  }
  save_annotation(ann);
}

/*
  add a new annotation to one graph
 */
function annotation_add_graph(g, p, ann) {
  var anns = g.annotations();
  if (p.annotation) {
    /* its an update */
    if (ann.text == '') {
      var idx = anns.indexOf(p);
      if (idx != -1) {
	anns.splice(idx,1);
      }
    } else {
      p.annotation.text = ann.text;
    }
  } else {
    anns.push(ann);
  }
  g.setAnnotations(anns);
}

/*
  add a new annotation
 */
function annotation_add(event, p) {
  var ann = {
  series: p.name,
  xval: p.xval - (tz_difference*60*60*1000),
  shortText: '!',
  text: prompt("Enter annotation", ""),
  };
  if (ann.text == '' || ann.text == null) {
    return;
  }
  for (var i=0; i<global_graphs.length; i++) {
    var g = global_graphs[i];
    if (g.series_names.indexOf(p.name) != -1) {
      annotation_add_graph(g, p, ann);
    }
  }

  save_annotation(ann);
}


/* default dygraph attributes */
defaultAttrs = {
 width: 700,
 height: 350,
 strokeWidth: 1,
 averaging: 1,
 annotationMouseOverHandler: annotation_highlight,
 annotationMouseOutHandler: annotation_unhighlight,
 annotationClickHandler: annotation_click,
 pointClickCallback: annotation_add
};

/*
  round to averaged time
 */
function round_time(t, n) {
  var t2 = t / (60*1000);
  t2 = Math.round((t2/n)-0.5);
  t2 *= n * 60 * 1000;
  return new Date(t2);
}

/*
  average some data over time
 */
function average_data(data, n) {
  var ret = new Array();
  var y;
  var counts = new Array();
  for (y=0; y<data.length; y++) {
    var t = round_time(data[y][0], n);
    if (ret.length > 0 && t.getTime() > ret[ret.length-1][0].getTime() + (6*60*60*1000)) {
      /* there is a big gap - insert a missing value */
      var t0 = ret[ret.length-1][0];
      var tavg = Math.round((t0.getTime()+t.getTime())/2);
      var t2 = new Date(tavg);
      var y2 = ret.length;
      ret[y2] = new Array();
      ret[y2][0] = t2;
      counts[y2] = new Array();
      for (var x=1; x<ret[y2-1].length; x++) {
	ret[y2][x] = null;
	counts[y2][x] = 0;
      }
    }
    var y2 = ret.length;
    if (ret.length > 0 && t.getTime() == ret[ret.length-1][0].getTime()) {
      var y2 = ret.length-1;
      for (var x=1; x<data[y].length; x++) {
	if (data[y][x] != null) {
	  ret[y2][x] += data[y][x];
	  counts[y2][x]++;
	}
      }
    } else {
      counts[y2] = new Array();
      ret[y2] = data[y];
      ret[y2][0] = t;
      for (var x=1; x<ret[y2].length; x++) {
	if (ret[y2][x] != null) {
	  counts[y2][x] = 1;
	}
      }
    }
  }
  for (y2=0; y2<ret.length; y2++) {
    for (var x=1; x<ret[y2].length; x++) {
      if (ret[y2][x] != null) {
	ret[y2][x] /= counts[y2][x];
      }
    }
  }
  return ret;
}

/*
  graph results from a set of CSV files:
    - apply func1 to the name columns within each file
    - apply func2 between the files
 */
function graph_csv_files_func(divname, filenames, columns, func1, func2, attrs) {
  /* load the csv files */
  var caller = new Object();
  caller.divname   = divname;
  caller.filenames = filenames.slice(0);
  caller.columns   = columns.slice(0);
  caller.func1     = func1;
  caller.func2     = func2;
  caller.attrs     = attrs;

  if (attrs.series_base != undefined) {
    caller.colname = attrs.series_base;  
  } else if (columns.length == 1) {
    caller.colname = columns[0]
  } else {
    caller.colname = divname;
  }

  /* called when all the data is loaded and we're ready to apply the
     functions and graph */
  function loaded_callback(d) {

    if (d[0] == undefined) {
      loading(false);
      return;
    }

    for (var i=0; i<caller.filenames.length; i++) {
      apply_function(d[i], caller.func1, caller.colname);
    }

    /* work out the y offsets to align the times */
    var yoffsets = new Array();
    for (var i=0; i<caller.filenames.length; i++) {
      yoffsets[i] = 0;
    }

    if (caller.attrs.missingValue !== undefined) {
      missingValue = attrs.missingValue;
    } else {
      missingValue = null;
    }
    
    /* map the data */
    var data = d[0].data;
    for (var y=0; y<data.length; y++) {
      if (data[y][1] == missingValue || data[y][1] == null) {
	data[y][1] = null;
      }
      for (var f=1; f<caller.filenames.length; f++) {
	var y2 = y + yoffsets[f];
	if (y2 >= d[f].data.length) {
	  y2 = d[f].data.length-1;
	}
	if (y2 < 0) {
	  y2 = 0;
	}
	while (y2 > 0 && d[f].data[y2][0] > data[y][0]) {
	  y2--;
	}
	while (y2 < (d[f].data.length-1) && d[f].data[y2][0] < data[y][0]) {
	  y2++;
	}
	yoffsets[f] = y2 - y;
	if (d[f].data.length <= y2 || 
	    d[f].data[y2][0] != data[y][0] || 
	    d[f].data[y2][1] == missingValue || 
	    d[f].data[y2][1] == null) {
	  data[y][f+1] = null;
	} else {
	  data[y][f+1] = d[f].data[y2][1];
	}
      }
    }
    
    labels = new Array();
    if (caller.colname.constructor == Array) {
      labels = caller.colname.slice(0);
    } else {
      labels[0] = d[0].labels[0];
      for (var i=0; i<caller.filenames.length; i++) {
	labels[i+1] = caller.colname + (i+1);
      }
    }

    var d2 = { labels: labels, data: data };
    apply_function(d2, caller.func2, caller.colname);
    
    /* add the labels to the given graph attributes */
    caller.attrs.labels = d2.labels;
    
    for (a in defaultAttrs) {
      if (caller.attrs[a] == undefined) {
	caller.attrs[a] = defaultAttrs[a];
      }
    }

    caller.attrs['labelsDiv'] = divname + ":labels";

    /* we need to create a new one, as otherwise we can't remove
       the annotations */       
    for (var i=0; i<global_graphs.length; i++) {
	var g = global_graphs[i];
	if (g.divname == divname) {
	  global_graphs.splice(i, 1);
	  g.destroy();
	  break;
	}
    }

    var max_points = 900;
    if (is_IE) {
      max_points = 100;
    }
    if (auto_averaging) {
      if (d2.data != null && (d2.data.length/defaultAttrs.averaging) > max_points) {
	var averaging_times = [ 1, 2, 5, 10, 15, 20, 30, 60, 120, 240, 480 ];
	var tdiff = 1;
	var num_minutes = (d2.data[d2.data.length-1][0] - d2.data[0][0]) / (60*1000);
	for (var i=0; i<averaging_times.length-1; i++) {
	  if (num_minutes / averaging_times[i] <= max_points) {
	    break;
	  }
	}
	set_averaging(averaging_times[i]);
	round_annotations();
      }
    }

    var avg_data;
    if (attrs.averaging == false) {
      avg_data = d2.data.slice(0);
      for (var y=0; y<avg_data.length; y++) {
	avg_data[y][0] = new Date(avg_data[y][0]);
      }
    } else {
      avg_data = average_data(d2.data, defaultAttrs.averaging);
    }

    if (attrs.maxtime != undefined) {
      var start = new Date() - (attrs.maxtime * 60 * 1000);
      var y;
      for (y=avg_data.length-1; y>0; y--) {
	if (avg_data[y][0] < start) {
	  break;
	}
      }    
      avg_data = avg_data.slice(y);
    }

    /* create a new dygraph */
    if (hashvars['nograph'] != '1') {
      g = new Dygraph(document.getElementById(divname), avg_data, caller.attrs);
      g.series_names = caller.attrs.labels;
      g.divname = divname;
      g.setAnnotations(annotations);
      global_graphs.push(g);
    }

    loading(false);
  }


  /* fire off a request to load the data */
  loading(true);
  heading(divname);
  graph_div(divname);

  function graph_callback(caller) {
    get_csv_data(caller.filenames, caller.columns, loaded_callback);
  }

  queue_graph(graph_callback, caller);
}


function product(v) {
  var r = v[0];
  for (var i=1; i<v.length; i++) {
    r *= v[i];
  }
  return r;
}

function sum(v) {
  var r = 0;
  for (var i=0; i<v.length; i++) {
    if (v[i] != null) {
      r += v[i];
    }
  }
  return r;
}



/*
  graph one column from a set of CSV files
 */
function graph_csv_files(divname, filenames, column, attrs) {
  return graph_csv_files_func(divname, filenames, [column], null, null, attrs);
}

/*
  graph one column from a set of CSV files as a sum over multiple files
 */
function graph_sum_csv_files(divname, filenames, column, attrs) {
  return graph_csv_files_func(divname, filenames, [column], null, sum, attrs);
}

/*
  called when the user selects a date
 */
function set_date(e) {
  var dp = datePickerController.getDatePicker("pvdate");
  pvdate = date_round(dp.date);
  hashvars['date'] = date_YMD(pvdate);
  rewrite_hashvars(hashvars);
  writeDebug("redrawing for: " + pvdate);
  annotations = new Array();
  show_graphs();
}

/*
  setup the datepicker widget
 */
function setup_datepicker() {
    document.getElementById("pvdate").value = 
      intLength(pvdate.getDate(),2) + "/" + intLength(pvdate.getMonth()+1, 2) + "/" + pvdate.getFullYear();
    datePickerController.addEvent(document.getElementById("pvdate"), "change", set_date);
}


/* 
   called to reload every few minutes
 */
function reload_timer() {
  /* flush the old CSV cache */
  CSV_Cache = new Array();
  writeDebug("reloading on timer");
  if (loading_counter == 0) {
    show_graphs();
  }
  setup_reload_timer();
}

/*
  setup for automatic reloads
 */
function setup_reload_timer() {
  setTimeout(reload_timer, 300000);    
}


/*
  toggle display of a div
 */
function toggle_div(divname)
{
  var div = document.getElementById(divname);
  var img = document.getElementById("img-" + divname);
  var current_display = div.style.display;
  var old_src = img.getAttribute("src");
  if (current_display != "none") {
    div.style.display = "none";
    img.setAttribute("src", old_src.replace("_unhide", "_hide"));
  } else {
    div.style.display = "block";
    img.setAttribute("src", old_src.replace("_hide", "_unhide"));
  }
}

/*
  change display period
 */
function change_period(p) {
  p = +p;
  if (period_days != p) {
    period_days = p;
    auto_averaging = 1;
    set_averaging(1);
    show_graphs();
  }
}

/*
  change averaging
 */
function change_averaging() {
  var v = +document.getElementById('averaging').value;
  defaultAttrs.averaging = v;
  auto_averaging = 0;
  show_graphs();
}

/*
  change averaging
 */
function set_averaging(v) {
  var a = document.getElementById('averaging');
  a.value = v;
  defaultAttrs.averaging = v;
}
