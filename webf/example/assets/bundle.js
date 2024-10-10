var text1 = document.createTextNode('Hello webf!');
var br = document.createElement('br');
var text2 = document.createTextNode('你好，webf！');
var p = document.createElement('p');
p.className = 'p';
p.style.display = 'inline-block';
p.style.textAlign = 'center';
p.style.animation = '3s ease-in 1s 1 reverse both running example';
p.appendChild(text1);
p.appendChild(br);
p.appendChild(text2);

document.body.appendChild(p);
var button = document.getElementById('button');

function testFormData(){
    // 创建一个 FormData 实例
    const formData = new FormData();
    console.log('formData',formData,'type=',formData.constructor.name)
    // 添加一些数据
    formData.append('key1', 'value1');
    formData.append('key1', 'value1.1');
    formData.append('key2', 'value2');
    formData.set('key2', 'new-value2');

    // 测试 get 方法
    console.log('Getting key1:', formData.get('key1'));
    console.log('Getting key2:', formData.get('key2'));

    // 测试 getAll 方法
    console.log('Getting all key1:', formData.getAll('key1'));

    console.log('Getting all key2:', formData.getAll('key2'));

    // 测试 has 方法
    console.log('Checking has key1:', formData.has('key1'));
    console.log('Checking has key2:', formData.has('key2'));

    // 测试 del 方法
    formData.delete('key1');
    console.log('Deleting key1: has key1 after delete:', formData.has('key1'));

    // 测试 forEach 方法
    formData.forEach((value, key) => {
        console.log(`Iterating: Key: ${key}, Value: ${value}`);
    });
}
  
// 当文档加载完成时，为按钮添加点击事件监听器（可选，因为已经在 HTML 中通过 onclick 添加了）  
document.addEventListener('DOMContentLoaded', function() {  
    testFormData();
});


