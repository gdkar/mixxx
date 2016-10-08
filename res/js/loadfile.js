.pragma library
var in_flight = [ ]
var load_file = function load_file(filename, callback) {
    console.log("loading file ",filename,"with callback ",callback)
    var xhr = new XMLHttpRequest()
    xhr.open("GET", filename)
    xhr.onreadystatechange = function() {
        if(xhr.readyState == XMLHttpRequest.DONE) {
            if(callback) try {
                callback(xhr.responseText.toString())
            }catch(any) {
                console.log(String(any))
            }
            in_flight.pop(xhr)
        }
    }
    in_flight.push(xhr)
    xhr.send()
    return xhr;
}
