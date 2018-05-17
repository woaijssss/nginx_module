https://blog.csdn.net/nestler/article/details/31825335
CORE_LIBS="$CORE_LIBS -lfoo"
1. 如果当前已安装nginx，首先运行sudo /etc/init.d/nginx stop，然后运行sudo apt remove nginx，然后再去sudo rm /usr/sbin/nginx，sudo rm -rf /usr/share/nginx, sudo rm -rf /etc/nginx
2. 从 http://nginx.org/download/ 下载一个nginx的版本，我用的是1.11.5
3. 把插件程序和config文件准备好，执行 ./configure --add-module=/home/licg/code/nginx_module/config_test, make和sudo make install
4. 修改/usr/local/nginx/conf/nginx.conf，加上location的修改
        location = /licg {
            test_command_name;
        }

5. 执行sudo /usr/local/nginx/sbin/nginx，启动nginx
6. 在浏览器中打开localhost/licg或127.0.0.1/licg，看到插件的作用






http://www.ttlsa.com/nginx/nginx-dynamic-modules/
这里提到加载动态模块，但命令仍然是需要执行./configure，即仍旧是需要源码？

/usr/sbin/nginx

nginx -V


https://stackoverflow.com/questions/36554405/how-to-enable-dynamic-module-with-an-existing-nginx-installation

https://www.nginx.com/resources/wiki/extending/converting/

https://www.nginx.com/blog/creating-installable-packages-dynamic-modules/


https://www.nginx.com/blog/compiling-dynamic-modules-nginx-plus/
https://www.nginx.com/blog/nginx-dynamic-modules-how-they-work/#howTo
https://dzhorov.com/2017/04/compiling-dynamic-modules-into-nginx-centos-7

https://stackoverflow.com/questions/41784476/nginx-and-geoip2-geolite2-error-is-not-binary-compatible
