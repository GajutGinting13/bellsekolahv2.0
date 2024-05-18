var id = sessionStorage.getItem('sesson');
if(id !== "1"){
    alert("Harap Login Terlebih Dahulu");
    window.location.href = "index.html";
}
function logout(){
    var konfir = confirm("Apakah Anda Yakin Ingin Keluar??");
    if (konfir){
        alert('Berhasil Keluar');
        sessionStorage.setItem("sesson", "0");
        window.location.href = "index.html";
    }
}
function updateClock() {
    var now = new Date();
    var hours = now.getHours();
    var minutes = now.getMinutes();
    var tanggal = now.getDate();
    var bulan = now.getMonth() + 1;
    var seconds = now.getSeconds();
    var tahun = now.getFullYear();
    var time = hours.toString().padStart(2, '0') + ':' + minutes.toString().padStart(2, '0') + ':' + seconds.toString().padStart(2, '0');
    var hari = now.toLocaleDateString('id-ID', { weekday: 'long' });
    document.getElementById("clock").textContent = time;
    var datawaktu = {jam: hours, 
                    menit: minutes, 
                    tanggal: tanggal, 
                    bulan: bulan, 
                    detik: seconds, 
                    tahun: tahun, 
                    waktusekarang: time, 
                    hari: hari};
    return datawaktu;
}
function konversiKeFormat24Jam(data) {
    var inputWaktu = document.getElementById(data).value;
    var jamMenit = inputWaktu.split(":");
    var jam = parseInt(jamMenit[0], 10);
    var menit = jamMenit[1];
    var waktu24Jam = jam.toString().padStart(2, '0') + ':' + menit + ':' + '00'; 
    var hasilwaktu = {jam:jam,
                        menit:menit
    }
    return hasilwaktu;
}
setInterval(updateClock, 1000);
updateClock();