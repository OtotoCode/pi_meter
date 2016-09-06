'use strict';


const remote = require('electron').remote;
//ブラウザウィンドウの取得
const browserWindow = remote.getCurrentWindow();
//画面サイズ配列の取得　[0]が横幅、[1]が縦幅
const win_size = browserWindow.getSize();

//オープニングで使用するカウンター
var counter=0;
//オープニングで使用するカウンターの最大数
var animaton_counter=100;//10msec * 200 = 2seconds
//各メーターの移動オープニングで使用するカウンター
var counter1=0;
var counter2=0;
var counter3=0;
//各メーターの移動オープニングで使用するカウンターを進める値
var add_counter1 = 0; 
var add_counter2 = (win_size[0]/3)/animaton_counter; 
var add_counter3 = (win_size[0]/3*2)/animaton_counter; 

//カウンターの最大値＝固定地点
var count_max1 = 0;
var count_max2 = (win_size[0]/3);
var count_max3 = (win_size[0]/3*2);

//各丸メーター背景の取得
var a_wtmp = document.getElementById("a_wtmp");
var a_otmp = document.getElementById("a_otmp");
var a_opls = document.getElementById("a_opls");

//各文字メーター背景の取得
var d_wtmp = document.getElementById("d_wtmp");
var d_otmp = document.getElementById("d_otmp");
var d_opls = document.getElementById("d_opls");

//各文字メーター用の文字表示部の取得
var str_wtmp = document.getElementById("str_wtmp");
var str_otmp = document.getElementById("str_otmp");
var str_opls = document.getElementById("str_opls");

//各丸メーターの針画像の取得
var bar_wtmp = document.getElementById("b_wtmp");
var bar_otmp = document.getElementById("b_otmp");
var bar_opls = document.getElementById("b_opls");

//各メーター用針の表示角度
var rt_wtmp=0;
var rt_otmp=0;
var rt_opls=0;

//水温の取得用変数と一時保存用変数
var tmp_wtmp1=0;
var tmp_wtmp2=0;

//油温の取得用変数と一時保存用変数
var tmp_otmp1=0;
var tmp_otmp2=0;

//油圧の取得用変数と一時保存用変数
var tmp_opls1=0;
var tmp_opls2=0;

//オープニグで使用する折り返しフラグ
var turn_flg=0;

//メーター用データファイルの取得
var fs = require('fs');
var filepath = './meter_data.json';

//ウィンドウの最小化
document.getElementById("im_opls").addEventListener("click", function (e) {
	browserWindow.minimize(); 
});

//終了処理
document.getElementById("im_wtmp").addEventListener("click", function (e) {
	browserWindow.close();
}); 

//丸メーターの移動
function mv_analog_p(){
	a_wtmp.style.left = 0+'px';
	a_wtmp.style.top= 0+'px';

	a_otmp.style.left = counter2+'px';
	a_otmp.style.top= 0+'px';

	a_opls.style.left = counter3+'px';
	a_opls.style.top= 0+'px';
}

//文字メーターの移動
function mv_dgital_p(){
	d_wtmp.style.left = 0+'px';
	d_wtmp.style.top= (win_size[0]/3)+'px';
	
	d_otmp.style.left = counter2+'px';
	d_otmp.style.top= (win_size[0]/3)+'px';
 
	d_opls.style.left = counter3+'px';
	d_opls.style.top= (win_size[0]/3)+'px';
}

//モジメーター用表示部の移動
function mv_dgital_str(){
	str_wtmp.style.right = (counter3+(win_size[0]/12))+'px';
	str_wtmp.style.top= (win_size[0]/24*9)+'px';
	str_wtmp.style.zIndex = '4';
	str_wtmp.style.fontSize=(win_size[0]/8)+"px";

	str_otmp.style.right = (counter2+(win_size[0]/12))+'px';
	str_otmp.style.top= (win_size[0]/24*9)+'px';
	str_otmp.style.zIndex = '4';
	str_otmp.style.fontSize=(win_size[0]/8)+"px";

	str_opls.style.right = (win_size[0]/12)+'px';
	str_opls.style.top= (win_size[0]/24*9)+'px';
	str_opls.style.zIndex = '4';
	str_opls.style.fontSize=(win_size[0]/8)+"px";
}

//丸メーター針の移動
function mv_analog_bar(){
	bar_wtmp.style.left = 0+'px';
	bar_wtmp.style.top= 0+'px';
	bar_wtmp.style.zIndex = '5';
	bar_wtmp.style.webkitTransform = "rotate("+rt_wtmp+"deg)";

	bar_otmp.style.left = count_max2+'px';
	bar_otmp.style.top= 0+'px';
	bar_otmp.style.zIndex = '5';
	bar_otmp.style.webkitTransform = "rotate("+rt_otmp+"deg)";

	bar_opls.style.left = count_max3+'px';
	bar_opls.style.top= 0+'px';
	bar_opls.style.zIndex = '5';
	bar_opls.style.webkitTransform = "rotate("+rt_opls+"deg)";
}

//オ－プニングの描画用
function draw_data(){
	bar_wtmp.style.webkitTransform = "rotate("+rt_wtmp+"deg)";
	document.getElementById("str_wtmp").innerHTML = (rt_wtmp/2.25).toFixed(0);
	bar_otmp.style.webkitTransform = "rotate("+rt_otmp+"deg)";
	document.getElementById("str_otmp").innerHTML = (rt_otmp/1.8).toFixed(0);
	bar_opls.style.webkitTransform = "rotate("+rt_opls+"deg)";
	document.getElementById("str_opls").innerHTML = (rt_opls/27).toFixed(1);
}

//パネルの移動オープニング
function start_meter(){
	if(counter<animaton_counter){
		mv_analog_p();
		mv_dgital_p();
		counter2=counter2+add_counter2;
		counter3=counter3+add_counter3;
		counter++;
		setTimeout('start_meter()',10);
	}else{
		counter1=count_max1;
		counter2=count_max2;
		counter3=count_max3;
		mv_analog_p();
		mv_dgital_p();
		mv_dgital_str();
		mv_analog_bar();
		opening();
	}
}

//オープニング
function opening() {
	if( (rt_wtmp!=270) && (turn_flg==0) ){
		rt_wtmp+=2;
		rt_otmp+=2;
		rt_opls+=2;
		draw_data();
		setTimeout("opening()",10);
	}else if( (rt_wtmp==270) && (turn_flg==0)  ){
		turn_flg=1;
		setTimeout("opening()",200);
	}else if((rt_wtmp!=0) && (turn_flg==1)){
		rt_wtmp-=2;
		rt_otmp-=2;
		rt_opls-=2;
		draw_data();
		setTimeout("opening()",10);
	}else if ((rt_wtmp==0) && (turn_flg==1)){
		turn_flg=0;
		setTimeout("mainloop()",15);
	}
}

//データの読み出し
function check_data() {
	var obj = JSON.parse(fs.readFileSync(filepath, 'utf8'));
	tmp_wtmp1=parseInt(obj["wtmp"]);
	tmp_otmp1=parseInt(obj["otmp"]);
	tmp_opls1=parseInt(obj["opls"]);
}

//テスト用　check_data()　の代わり　今は使用していない
function test_data() {
	if(turn_flg==0){
		tmp_wtmp1++;
		tmp_otmp1++;
		tmp_opls1++;
	}else{
		tmp_wtmp1--;
		tmp_otmp1--;
		tmp_opls1--;
	}
	if(tmp_wtmp1==100){
		turn_flg=1;
	}else if(tmp_wtmp1==0){
		turn_flg=0;
	}
}

//メインループ
function mainloop(){
	check_data();
	if(tmp_wtmp1!=tmp_wtmp2){
		if(tmp_wtmp1 > tmp_wtmp2){
			tmp_wtmp2++;
		}else{
			tmp_wtmp2--;
		}
		rt_wtmp=tmp_wtmp2*2.25;
		bar_wtmp.style.webkitTransform = "rotate("+rt_wtmp+"deg)";
		document.getElementById("str_wtmp").innerHTML = tmp_wtmp2 ;
	}
		
	if(tmp_otmp1!=tmp_otmp2){
		if(tmp_otmp1 > tmp_otmp2){
			tmp_otmp2++;
		}else{
			tmp_otmp2--;
		}
		rt_otmp=tmp_otmp2*1.8;
		bar_otmp.style.webkitTransform = "rotate("+rt_otmp+"deg)";
		document.getElementById("str_otmp").innerHTML =tmp_otmp2;
	}

	if(tmp_opls1!=tmp_opls2){
		if(tmp_opls1 > tmp_opls2){
			tmp_opls2++;
		}else{
			tmp_opls2--;
		}
		rt_opls=tmp_opls2*2.7;
		bar_opls.style.webkitTransform = "rotate("+rt_opls+"deg)";
		document.getElementById("str_opls").innerHTML =(tmp_opls2/10).toFixed(1);
	}
	setTimeout("mainloop()",15);
}


window.onload = function(){
	start_meter();
}