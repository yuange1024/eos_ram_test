# EOS RAM 测试

## 代码说明
为了方便测试,从 https://github.com/EOSIO/eos 中提取了相关代码,涉及到的代码文件如下:
- [contracts/eosio.system/eosio.system.cpp](https://github.com/EOSIO/eos/blob/v1.0.9/contracts/eosio.system/eosio.system.cpp)
- [contracts/eosio.system/delegate_bandwidth.cpp](https://github.com/EOSIO/eos/blob/v1.0.9/contracts/eosio.system/delegate_bandwidth.cpp)
- [contracts/eosio.system/exchange_state.hpp](https://github.com/EOSIO/eos/blob/v1.0.9/contracts/eosio.system/exchange_state.hpp)
- [contracts/eosio.system/exchange_state.cpp](https://github.com/EOSIO/eos/blob/v1.0.9/contracts/eosio.system/exchange_state.cpp)
- [contracts/exchange/test_exchange.cpp](https://github.com/EOSIO/eos/blob/v1.0.9/contracts/exchange/test_exchange.cpp)

## 编译和执行测试
```
g++ -stdlib=libc++ -std=c++17 -o test_exchange test_exchange.cpp

./test_exchange
```

## 测试结果说明

### 当前(2018-07-12)主网参数下的测试结果
- 从 0% 到 99% 的测试结果,间隔 1% : https://github.com/yuange1024/eos_ram_test/blob/master/ram_price_0.00_0.99.txt
- 从 0.00% 到 99.00% 的测试结果,间隔 0.01% :  https://github.com/yuange1024/eos_ram_test/blob/master/ram_price_0.0001_0.9900.txt

### 修改部分参数的测试结果
测试结果: https://github.com/yuange1024/eos_ram_test/blob/master/test_result.txt

测试表明,修改 bancor 参数中的 weight 并不影响结果。 
 
将初始内存修改为128G时, 达到相同内存已购比例时,如在初始内存64G时已购买16G,在初始内存128G时已购买32G, 扩容2倍后的价格约为原先的二分之一,扩容4倍后,同等已购内存比例价格约为原先的四分之一。

需要说明的是,目前(2018-07-12)扩容方案并未确定,本项目中直接修改初始内存数量不是实际扩容方案,仅是为了查看不同初始条件下的价格变化。

BM 在2018年7月4日提交了相关代码 https://github.com/EOSIO/eosio.contracts/pull/2 , 代码中的扩容方案是随着区块高度逐步扩容。如果按照该方案进行逐步扩容, 对价格的影响也是逐步的,不会出现价格突变,感兴趣的朋友可以做一下相关测试。
 
## 其它
- [猿哥翻译的《Bancor Protocol》](https://mp.weixin.qq.com/s/b_iekZpjF-oR2Gvm-5Ljsw)
- EOS RAM 导航网站  https://eosram.vip
