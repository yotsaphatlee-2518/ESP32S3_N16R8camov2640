const BtnAdd = document.getElementById("set_btn");
const DivContainer = document.getElementById("network_select_buttons");
const ssidInput = document.getElementById("ssid");
const passwordInput = document.getElementById("password");
const saveBtn = document.getElementById("save_btn");
const refreshBtn = document.getElementById("refresh_btn");
const apStatus = document.getElementById("ap_status");
const ipStatus = document.getElementById('ip_status');

const wifi_auth_mode = Object.freeze({
  0: "OPEN",
  1: "WEP",
  2: "WPA_PSK",
  3: "WPA2_PSK",
  4: "WPA_WPA2_PSK",
  5: "Wifi EAP Security",
  6: "Wifi EAP Security",
  7: "WPA3_PSK",
  8: "WPA2_WPA3_PSK",
  9: "WAPI_PSK",
  10: "OWE",
  11: "WPA3_ENT_SUITE_B_192_BIT",
  12: "WPA3_PSK_EXT_KEY",
  13: "WPA3_PSK + WPA3_PSK_EXT_KEY",
  14: "MAX"
});

/*!
 * Initialize functions here.
 */
$(document).ready(function () {
  getWifiNetworks();
  getAPStatus();
});

/*!
* Add new button.
*/
function AddNew(ssid, authmode, rssi) {
  const newbtn = document.createElement("button");
  newbtn.classList.add("btn");
  newbtn.type = "button";
  newbtn.innerHTML = ssid +
    (authmode !== "OPEN" ? ' <span style="font-size:1.2em">&#128274;</span>' : '') +
    ' <span style="font-size:1.2em">&#128246;</span>';
  newbtn.onclick = function () {
    ssidInput.value = ssid;
    passwordInput.value = "";
  }
  DivContainer.appendChild(newbtn);
}

/*!
* Gets the wifi networks.
*/
function getWifiNetworks() {
  DivContainer.innerHTML = "";
  var xhr = new XMLHttpRequest();
  var requestURL = "/listofScannedWifiNetworks";
  xhr.open('POST', requestURL, false);
  xhr.send('listofScannedWifiNetworks');

  if (xhr.readyState == 4 && xhr.status == 200) {
    var response = JSON.parse(xhr.responseText);
    // เรียงลำดับ rssi จากมากไปน้อย (ใกล้ค่ายสุด)
    response.ap_records.sort(function(a, b) {
      return b.rssi - a.rssi;
    });
    // แสดงแค่ 5 รายการแรก
    for (var i = 0; i < Math.min(5, response.ap_records.length); i++) {
      AddNew(response.ap_records[i].ssid, getWifiSecurityType(response.ap_records[i].authmode), response.ap_records[i].rssi);
    }
  }
}

/*!
* Gets the wifi networks list
*/
function getWifiNetworksList(list_size, responseTEXT) {

  var response = JSON.parse(responseText);

  console.log(response);



}

/*!
* Wifi Securtiy Type
*/
function getWifiSecurityType(authmode) {
  return wifi_auth_mode[authmode];
}

// ปุ่ม Refresh
refreshBtn.addEventListener("click", function (e) {
  e.preventDefault();
  getWifiNetworks();
});

// ปุ่ม Save
saveBtn.addEventListener("click", function (e) {
  e.preventDefault();
  var ssid = ssidInput.value;
  var password = passwordInput.value;
  if (!ssid) {
    alert("Please enter SSID");
    return;
  }
  var xhr = new XMLHttpRequest();
  xhr.open('POST', '/connectWifi.json', true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.setRequestHeader('ConnectSSID', ssid);
  xhr.setRequestHeader('ConnectPassword', password);
  xhr.onreadystatechange = function () {
    if (xhr.readyState == 4 && xhr.status == 200) {
      alert("Saved! Please wait for connection.");
      getAPStatus();
    }
  };
  xhr.send(JSON.stringify({ ssid: ssid, password: password }));
});

// แสดงสถานะ AP ที่เชื่อมต่อ
function getAPStatus() {
  var xhr = new XMLHttpRequest();
  xhr.open('POST', '/wifiConnectStatus', true);
  xhr.onreadystatechange = function () {
    if (xhr.readyState == 4 && xhr.status == 200) {
      var response = JSON.parse(xhr.responseText);
      if (response.status === 1) {
        apStatus.textContent = "";
        if (response.ip && response.mask && response.gw && ipStatus) {
          ipStatus.innerHTML = "เชื่อมต่อสำเร็จ : <b>ip</b>: " + response.ip +
            ", <b>mask</b>: " + response.mask +
            ", <b>gw</b>: " + response.gw;
        }
      } else {
        apStatus.textContent = "No AP set";
        if (ipStatus) ipStatus.textContent = "";
      }
    }
  };
  xhr.send();
}

// แสดง/ซ่อนรหัสผ่าน
const showPasswordCheckbox = document.getElementById('showPassword');
if (showPasswordCheckbox) {
  showPasswordCheckbox.addEventListener('change', function() {
    passwordInput.type = this.checked ? 'text' : 'password';
  });
}

