
var myFCoins = {{ g.user.fcoins if g.user.fcoins > 0 else 0 }};

var div = document.getElementById("myFCoins");
if (div != null) {
    div.innerHTML = myFCoins;
}
