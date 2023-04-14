## Actualizacion

En la Wiki se actualizarán constantemente los datos que son modificables en cada versión. Si uno de los datos no se encuentra en su sección, por favor, ignórelo


# Informacion general del proyecto

Este proyecto trata de crear un invernadero inteligente mediante el uso de un microcontrolador ESP32 WROOM. El objetivo principal del invernadero es controlar la iluminación y la ventilación de forma automática. Además, se envían los datos de los sensores a ThingSpeak mediante WiFi y a una página web local.

El código del proyecto incluye la inicialización de los pines utilizados por los sensores y relés, la definición de los tiempos de actualización de los diferentes componentes, la configuración de la conexión WiFi, la lectura de los sensores y el control de la iluminación y la ventilación. Se utilizan varios sensores, entre ellos el MQ-9 para detectar la calidad del aire, el BHT22 para medir la humedad y la temperatura, el LM35DZ para medir la temperatura ambiente, y un sensor de humedad del suelo para medir la humedad del sustrato. Además, se ha integrado el sensor BMP280 de presión y temperatura, aunque actualmente se encuentra deshabilitado en el código.

El sensor MQ-9 es un sensor de gas que mide la concentración de monóxido de carbono, hidrógeno y metano en el aire. Es muy útil para detectar la calidad del aire en el invernadero y tomar medidas para mejorarla.

El sensor BHT22, también conocido como DHT22, mide la humedad relativa y la temperatura ambiente. Es un sensor muy preciso y confiable, y es ideal para controlar el ambiente dentro del invernadero.

El sensor de humedad del suelo es muy importante para medir la humedad del sustrato y controlar el riego de las plantas en el invernadero. Es un sensor resistivo que mide la humedad del suelo y envía una señal al microcontrolador para que se tomen medidas en consecuencia.

En este fascinante proyecto de invernadero inteligente, se ha integrado el sensor BMP280 de Presión y Temperatura. A pesar de estar deshabilitado en el código, este sensor es muy útil ya que permite medir la presión atmosférica y la temperatura ambiente.

En conclusión, este proyecto de invernadero inteligente es muy útil y práctico para controlar el ambiente de las plantas en un espacio cerrado. La integración de varios sensores permite tomar medidas precisas y mejorar la calidad del aire, la humedad y la temperatura. Además, el uso de ThingSpeak y una página web local permite monitorear y controlar el invernadero desde cualquier lugar.
