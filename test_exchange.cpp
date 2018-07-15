#include <string>
#include <cstdint>
#include <iostream>
#include <cmath>
#include <cassert>
using namespace std;
typedef long double real_type;
typedef double token_type;

void eosio_assert( bool test, const string& msg ) 
{
   if( !test ) throw std::runtime_error( msg );
}

typedef string  symbol_type;

struct asset 
{
   real_type amount;
   symbol_type symbol;
};

struct exchange_state 
{
    asset    supply;
    struct connector
    {
        asset balance;
        double weight = .5;
        
    };
    real_type ram_init_amount;
    connector base;
    connector quote;
    
    void print_state()
    {
        printf("exchange_state: [supply.amount:%.0Lf][base.amount:%.0Lf][quote.amount:%.0Lf][ram_init_amount:%.0LfG][weight:%.1f]\n",
               supply.amount, base.balance.amount, quote.balance.amount,ram_init_amount / (1024 * 1024 * 1024),base.weight);
    }
    
    asset convert_to_exchange( connector& c, asset in )
    {
        real_type R(supply.amount);
        real_type C(c.balance.amount+in.amount);
        real_type F(c.weight/1000.0);
        real_type T(in.amount);
        real_type ONE(1.0);

        real_type E = -R * (ONE - std::pow( ONE + T / C, F) );
        int64_t issued = int64_t(E);

        supply.amount += issued;
        c.balance.amount += in.amount;

        asset result ;
        result.amount = issued;
        result.symbol = supply.symbol;
        return result;
    }
    
    asset convert_from_exchange( connector& c, asset in )
    {
        eosio_assert( in.symbol == supply.symbol, "unexpected asset symbol input" );
        real_type R(supply.amount - in.amount);
        real_type C(c.balance.amount);
        real_type F(1000.0/c.weight);
        real_type E(in.amount);
        real_type ONE(1.0);
      
        real_type T = C * (std::pow( ONE + E/R, F) - ONE);
        int64_t out = int64_t(T);
        supply.amount -= in.amount;
        c.balance.amount -= out;

        asset result ;
        result.amount = out;
        result.symbol = c.balance.symbol;
        return result;
    }
    
    // 注释中以将 EOS 转换为 RAM为例分析
    // from: EOS, to: RAM
    asset convert( asset from, symbol_type to )
    {
        auto sell_symbol  = from.symbol; // EOS
        auto ex_symbol    = supply.symbol; // RAMCORE
        auto base_symbol  = base.balance.symbol; // RAM
        auto quote_symbol = quote.balance.symbol; // EOS

        if( sell_symbol != ex_symbol ) 
        {
            if( sell_symbol == base_symbol ) 
            {
                from = convert_to_exchange( base, from );
            }
            else if( sell_symbol == quote_symbol )
            {
                from = convert_to_exchange( quote, from );  // 执行后，from 的symbol 为 RAMCORE
            } 
            else 
            {
                eosio_assert( false, "invalid sell" );
            }
        }
        else 
        {
            if( to == base_symbol )
            {
                from = convert_from_exchange( base, from ); // 递归调用时，执行该行代码后，from 的 symbol 变为了 RAM
            }
            else if( to == quote_symbol )
            {
                from = convert_from_exchange( quote, from );
            }
            else
            {
                eosio_assert( false, "invalid conversion" );
            }
        }

        if( to != from.symbol ) 
        {
            return convert( from, to ); // 递归调用 convert(RAMCORE, RAM)
        }
        return from;
      }
};


exchange_state ex;


void print_buy_info(long long bytes, double eos_amount)
{
    double kbytes = bytes / 1024.0;
    double eos = eos_amount / 10000; // 系统用的EOS是整数表示的， 最小单位 = 0.0001 EOS
    
    printf("%.1f K RAM = %.6f G RAM =  %.1LfG *  %.3Lf%%  = %.0f EOS\n",
           kbytes, kbytes /1024 / 1024,
           ex.ram_init_amount / (1024 * 1024 * 1024),
           bytes * 100.0 /  ex.ram_init_amount , eos);
    
    printf("Average RAM Pirce  = %.4f EOS/KB\n",  eos / kbytes);
}

double get_price(long long bytes, double eos)
{
    double kbytes = bytes / 1024.0;
    return  eos / 10000 / kbytes ; // 系统用的EOS是整数表示的， 最小单位 = 0.0001 EOS
}


// 初始化 EOS RAM 相关参数
void init_exchange_state()
{
    // 中间货币
    ex.supply.amount = 100000000000000ll;
    ex.supply.symbol = "RAMCORE";
    
    // RAM
    ex.base.balance.amount = 64ll * 1024 * 1024 * 1024;
    ex.ram_init_amount = ex.base.balance.amount;
    ex.base.balance.symbol = "RAM";
    ex.base.weight = 0.5;

    // EOS
    ex.quote.balance.amount = 10000000000ll;
    ex.quote.balance.symbol = "EOS";
    ex.quote.weight = 0.5;
}


double get_current_price()
{
    asset input, output;

    // 买 1 EOS，计算价格
    input.symbol = "EOS";
    input.amount = 1 * 10000; //  乘以 10000 表示 1 EOS ， 因为 eos 代码中的 EOS 数量是用整数表示的, 一个最小单位是 0.0001 EOS
  
    output = ex.convert(input, "RAM" );
    double price = get_price(output.amount, input.amount);

    // 再转换回去，恢复现状
    ex.convert(output, "EOS");

    return price;
}

// 计算初始价格: 0.0149015 EOS/KB
void calc_init_price()
{
    init_exchange_state();
    std::cerr <<"init price: "  << get_current_price()  << endl;
}

void calc_1_99_percent_info()
{
    ex.print_state();

    asset input;
    input.symbol = "EOS";
    
    // eosio.ram 这个账户的 EOS 余额
    double eosioram_balance = (100000000 - 1000000) * 10000.0; // 目前线上代码，内存被购买 99% 时的eosio.ram账户余额，参考其它资料获得的这个值，也可以逐步测试出来
    
    input.amount = eosioram_balance;
    double ram_bought = ex.convert(input, "RAM" ).amount;
    print_buy_info(ram_bought,input.amount);

    printf("\nratio: 内存被购买比例\nprice: 内存价格，单位：EOS/KB\neos: eosio.ram 账户余额\nram_bought: 被购买的内存数量，单位：字节\n\n");
    printf("ratio\tprice\teos\tram_bought\n");
    printf("%.1Lf%%\t%.4f\t%.0f\t%.0f\n", ram_bought * 100 / ex.ram_init_amount,  get_current_price(),  eosioram_balance / 10000, ram_bought );

    input.symbol = "RAM";
    // 每次卖  1 % 的内存
    input.amount = ex.ram_init_amount / 100.0 ;
    for(int ratio = 98; ratio >= 0; ratio -= 1)
    {
        double eos =  ex.convert(input,"EOS").amount;
        //ex.prinf_state();
        eosioram_balance -= eos;
        ram_bought -= input.amount;
        printf("%.1Lf%%\t%.4f\t%.0f\t%.0f\n", ram_bought * 100 / ex.ram_init_amount,  get_current_price(),  eosioram_balance / 10000, ram_bought );
    }
    
    /*
    // 每次卖 0.01 % 的内存
    input.amount = 64.0 * 1024 * 1024 * 1024 / 10000.0 ;
    for(double ratio = 98.99; ratio >= 0; ratio -= 0.01)
    {
        double eos =  ex.convert(input,"EOS").amount;
        //ex.prinf_state();
        eosioram_balance -= eos;
        ram_bought -= input.amount;
        printf("%.2f%%\t%.4f\t%.0f\t%.0f\n", ram_bought * 100 / ex.ram_init_amount,  get_current_price(),  eosioram_balance / 10000, ram_bought );
    }
    */
}

void change_ram_from_64g_to_128g_at_80_percent()
{
    init_exchange_state();
    
    ex.print_state();
    
    asset input;
    input.symbol = "EOS";
    
    double eosioram_balance = 4000000 * 10000.0; // 初始 64G 内存，内存被购买 80% 时， eosio.ram 账户余额
    
    input.amount = eosioram_balance;
    double ram_bought = ex.convert(input, "RAM" ).amount;
    print_buy_info(ram_bought,input.amount);
    
    
    printf("\nratio: 内存被购买比例\nprice: 内存价格，单位：EOS/KB\neos: eosio.ram 账户余额\nram_bought: 被购买的内存数量，单位：字节\n\n");
    printf("ratio\tprice\teos\tram_bought\n");
    printf("%.1Lf%%\t%.4f\t%.0f\t%.0f\n", ram_bought * 100 / ex.ram_init_amount,  get_current_price(),  eosioram_balance / 10000, ram_bought );
    
    
    // 在当前条件下扩容1倍，扩容后内存已售比例由80%变为40%
    ex.base.balance.amount += (64.0 * 1024 * 1024 * 1024);
    ex.ram_init_amount = (128.0 * 1024 * 1024 * 1024);
    
    ex.print_state();
    
    double eosioram_balance2 = (300000000 - 5000000)  * 10000.0; // 扩容到128G内存后，内存被购买 80% 时，再使用这么多EOS买成内存，会让内存已购比例达到99%
    
    input.amount = eosioram_balance2;
    double ram_bought2 = ex.convert(input, "RAM" ).amount;
    print_buy_info(ram_bought2,input.amount);
    print_buy_info(ram_bought + ram_bought2,input.amount);

    double current_eos = eosioram_balance + eosioram_balance2;
    double current_ram_bought = ram_bought2 + ram_bought;
    
    printf("%.3Lf%%\t%.4f\t%.0f\t%.0f\n",  current_ram_bought * 100 / ex.ram_init_amount,  get_current_price(),  current_eos / 10000, current_ram_bought );


    input.symbol = "RAM";
    // 每次卖  1 % 的内存
    input.amount = ex.ram_init_amount / 100.0 ;
    for(int ratio = 98; ratio >= 0; ratio -= 1)
    {
        double eos =  ex.convert(input,"EOS").amount;
        //ex.prinf_state();
        current_eos -= eos;
        current_ram_bought -= input.amount;
        printf("%.3Lf%%\t%.4f\t%.0f\t%.0f\n", current_ram_bought * 100 / ex.ram_init_amount,
               get_current_price(),  current_eos / 10000, current_ram_bought );
    }
}

// 测试结果表明，weight的值为0.5、5、50、500时，在RAM已售比例相同时，价格是是相同的
// EOS RAM 引入了一个中间货币RAMCORE, 这个测试结果和bancor的常规情况的确不同
void test_different_weight()
{
    double weight = 5;

    init_exchange_state();
    ex.base.weight = weight;
    ex.quote.weight = weight;
    printf("\n\n===============test weight = %.1f\n",weight);
    calc_1_99_percent_info();
    
    init_exchange_state();
    weight = 50;
    ex.base.weight = weight;
    ex.quote.weight = weight;
    printf("\n\n===============test weight = %.1f\n",weight);
    calc_1_99_percent_info();
    
    init_exchange_state();
    weight = 500;
    ex.base.weight = weight;
    ex.quote.weight = weight;
    printf("\n\n===============test weight = %.1f\n",weight);
    calc_1_99_percent_info();
}

void test_different_init_ram()
{
    double init_ram = 128.0;
    double ONEG = 1024.0 * 1024 * 1024;
    
    printf("\n\n===============test init_ram = %.0fG\n",init_ram);
    init_ram *= ONEG;
    init_exchange_state();
    ex.base.balance.amount = init_ram;
    ex.ram_init_amount = init_ram;
    calc_1_99_percent_info();
    
    init_ram = 256.0;
    printf("\n\n===============test init_ram = %.0fG\n",init_ram);
    init_ram *= ONEG;
    init_exchange_state();
    ex.base.balance.amount = init_ram;
    ex.ram_init_amount = init_ram;

    calc_1_99_percent_info();

    init_ram = 512.0;
    printf("\n\n===============test init_ram = %.0fG\n",init_ram);
    init_ram *= ONEG;
    init_exchange_state();
    ex.base.balance.amount = init_ram;
    ex.ram_init_amount = init_ram;
    calc_1_99_percent_info();
}

int main( int argc, char** argv ) 
{
    change_ram_from_64g_to_128g_at_80_percent();
    
    
    init_exchange_state();
    calc_1_99_percent_info();
    
    return 0;

    test_different_weight();
    
    test_different_init_ram();

    return 0;
}
